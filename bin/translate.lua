#!/usr/bin/env lua

local tpl = require "commons.template"
local dump = require 'pl.pretty'.dump
local mime = require("mime")
local fname = arg[1]
local var_name = arg[2]
local target_fname= arg[3]

if #arg ~=3 then
  fmt_tpl="Usage: {{&prog}}: script-fname var-name target-fname"
  line = tpl:render(fmt_tpl, {prog=arg[0]})
  print(line)
  os.exit(1)
end
assert(fname)
assert(var_name, "var_name")
--assert(fname_target, "fname_target")

fmt_target = [[
#define {{ &var_name }} \
{{&lines}} ""
]]
local fmt_line = [[ "{{ &line }}" \ 
]]

local fd=io.open(fname, "r")
assert(fd)
local content = fd:read("*a")
local content_base64 = mime.b64(content)
local lines={}
for pos=1, #content_base64, 72
  do
    line = content_base64:sub(pos, pos+72-1)
    line = tpl:render(fmt_line,{line=line})
    table.insert(lines, line)
  end
lines=table.concat(lines, '')
--dump(lines)

target=tpl:render(fmt_target, {var_name=var_name,lines=lines})
target_fd = io.open(target_fname, "w")
target_fd:write(target)
target_fd:close()
print(target)
