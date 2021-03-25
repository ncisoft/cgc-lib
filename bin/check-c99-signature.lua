#!/usr/bin/env luajit

--local tpl = require "lustache"
local tpl = require "commons.template"
require("commons.print")
local dump = require 'pl.pretty'.dump
local colors = require 'term.colors'
elf_fname = arg[1] or './build/test/gc-test'

--elf_fname = '/home/leeyg/tmp/dirty/foo.o'
cmd_objdump = 'objdump -S {{ &elf_fname }}'
cmd_nm = [[nm {{&elf_fname}} |grep "t \|T "|cut -d ' ' -f 3]]
text_section = "Disassembly of section .text:"
other_section = "Disassembly of section"

if #arg ~=1 then
  fmt_tpl="{{ colors.yellow }}Usage: {{&prog}}:  target-fname{{ colors.reset }}"
  line = tpl:render(fmt_tpl, {prog=arg[0], colors=colors})
  print(line)
  os.exit(1)
end
local M={}
local L =
{
  flag =
  {
    dump_signature = false,
  }
}

local function check()
  local matched=false
  local text_lines = {}
  local functions ={}
  local nm_functions = {}

  cmd=tpl:render(cmd_nm, {elf_fname=elf_fname})
  local fd = io.popen(cmd)
  assert(fd, "fail exec open " ..cmd)
  for line in fd:lines() do
    nm_functions[ line ] = true
  end

  cmd=tpl:render(cmd_objdump, {elf_fname=elf_fname})
  local fd = io.popen(cmd)
  assert(fd, "fail when open " ..elf_fname)
  for line in fd:lines() do
    if M.startWith(line, text_section) then
      matched = true
    elseif matched then
      if not M.startWith(line, other_section) then
        table.insert(text_lines, line )
      end
    end
  end

  -- process text section
  local _f = {}
  for _,line in ipairs(text_lines) do
    if M.startWith(line, "000000") then
      local start_i, end_j, str = line:find("%<([a-zA-Z0-9_]+)%>:")
      start_i, end_j, str = line:find("%<([a-zA-Z0-9_]+)%>:")
      if start_i and nm_functions[ str ] then
        _f = {name = str, lines={}}
        table.insert(functions, _f)
      end
      elseif _f.name then
        table.insert(_f.lines,line)
    end
  end

  -- functions = { name, lines{} }
  -- stripe
  for _, _f in ipairs(functions) do
    local flag = false
    local lines={}
    for i,line in ipairs(_f.lines) do
      if line:find("^%s+") or #line ==0 then
        --
      else
        table.insert(lines, line)
      end
    end
    _f._lines = _f.lines
    _f.lines = lines
  end
  -- dump(functions)

  -- process functions signature
  local _functions = {}
  for _, _f in ipairs(functions) do
    local has_signature = false
    for i,line in ipairs(_f.lines) do
      if not has_signature and line:find(_f.name ..'%(') then
        has_signature = true
        assert(_f.lines)
        _f.signature = line
        table.insert(_functions, _f)
      end
    end

    if has_signature then
      _f.lines=nil
      for _,line in ipairs(_f._lines) do
        local start_i, end_j, str = line:find("^%s+(using_raii_[0-9a-zA-Z_]+)")
        if start_i then
          _f.raii=str
        end
        _f._lines=nil
      end
    end

  end
  functions = _functions

  for _,_f in ipairs(functions) do
    if _f.signature then
      _f.lines = nil
    end
  end
  --dump(functions)

  _functions = {}
  -- process signature
  for _, _f in ipairs(functions) do
    local signature = _f.signature
    if signature then
      assert(signature)
      if M.startWith(signature, "extern ") then
        signature = signature:sub(#"extern "+1)
        assert(signature)
      end
      local start_i, end_j, str = signature:find("%s*([0-9a-zA-Z_]+)%(")
      if start_i then
        str = signature:sub(1, start_i -1)
        --print("... "..signature)
        --print("...... "..str .."..")
        local return_type = signature:sub(1, start_i-1)
        local has_asterisk=nil
        if return_type:sub(#return_type) == '*' then
          return_type = return_type:sub(1, #return_type -1)
          return_type = M.trim_right( return_type )
          has_asterisk = true
        end
        _f.return_type = return_type
        _f.has_asterisk = has_asterisk
        _f.is_primitive =   M.is_primitive(_f.return_type, _f.has_asterisk)
        table.insert(_functions, _f)
      end

    end
  end
  functions = _functions

  --dump(functions)
  -- process check return_type
  local nerror=0
  for _, _f in ipairs(functions) do
    if _f.raii == "using_raii_proot" and not _f.is_primitive then
      fmt= colors.yellow .."%s" .. colors.reset .." expect " ..colors.red .."using_raii_proot_complex_return " ..colors.reset
      print(fmt:format(_f.signature))
      dump(_f)
      nerror = nerror+1
      --os.exit(1)
    elseif _f.raii == "using_raii_proot_complex_return" and _f.is_primitive then
      fmt=colors.yellow .. "%s expect using_raii_proot " ..colors.reset
      fmt= colors.yellow .."%s" .. colors.reset .." expect " ..colors.red .."using_raii_proot" ..colors.reset
      print(fmt:format(_f.signature))
      dump(_f)
      nerror = nerror+1
      --os.exit(1)
    end
  end

  for _, _f in ipairs(functions) do
    local signature = _f.signature
    local start_i, end_j, str = signature:find("([0-9a-zA-Z_]+)%(")
    local _func = signature:sub(start_i, end_j-1)
    _f.func_name = _func
  end

  if L.flag.dump_signature then
    for _, _f in ipairs(functions) do
      local fmt='struct %s_meta = {.func="%s", .is_primitive=%s};'
      local line=sprint(fmt,_f.func_name,  _f.func_name, tostring(_f.is_primitive))
      print (line)
    end
  end

  if nerror > 0 then
    print( colors.yellow .. "found Err " ..nerror ..colors.reset )
    --os.exit(1)
  end

  --dump(functions)
  msg="{{{colors.yellow}}} {{{arg0}}} {{{elf_fname}}} is clean {{{colors.reset}}}"
  print(tpl:render(msg, {elf_fname=elf_fname, colors=colors, arg0=arg[0]}))
  os.exit(0)
end

M.startWith = function(s, start)
  if s:sub(1, #start) == start then
    return true
  end
  return false
end

local primitives =
{
  -- primitive
  "int",
  "char",
  "short",
  "long",
  "long long",
  "float",
  "double",


   -- signed
  "signed int",
  "signed char",
  "signed short",
  "signed long",
  "signed long long",

   -- unsigned
  "unsigned int",
  "unsigned char",
  "unsigned short",
  "unsigned long",
  "unsigned long long",

  -- unsigned
  -- stdint.h
  "uint8_t",
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "__ssize_t",

  "bool",
  "__ssize_t*",
  "void"
}

local primitive_map = {}
for _,type in ipairs(primitives) do
  primitive_map[ type ] = true
end

M.is_primitive = function(type, has_asterisk)
  if has_asterisk then
    return false
  end
  if primitive_map[type] then
    return true
  end
  return false
end

M.trim_right = function(str)
  assert(str)
  local start_i, end_j, s = str:find("%s*$")
  if start_i then
    str = str:sub(1, start_i-1)
  end
  return str
end


check()
