#!/usr/bin/env lua
package.cpath = package.cpath .. ';../build/?.so'

-- clang -cc1 -ast-dump your_file.c
-- clang -Xclang -ast-dump -fsyntax-only your_file.c

local clang = require 'luaclang-parser'
--local sqlite3 = require 'luasql.sqlite3'.sqlite3

-- from http://stevedonovan.github.com/Penlight/api/modules/pl.text.html#format_operator
do
    local format = string.format

    -- a more forgiving version of string.format, which applies
    -- tostring() to any value with a %s format.
    local function formatx (fmt,...)
        local args = {...}
        local i = 1
        for p in fmt:gmatch('%%.') do
            if p == '%s' and type(args[i]) ~= 'string' then
                args[i] = tostring(args[i])
            end
            i = i + 1
        end
        return format(fmt,unpack(args))
    end

    -- Note this goes further than the original, and will allow these cases:
    -- 1. a single value
    -- 2. a list of values
    getmetatable("").__mod = function(a, b)
        if b == nil then
            return a
        elseif type(b) == "table" then
            return formatx(a,unpack(b))
        else
            return formatx(a,b)
        end
    end
end

do
    local start = os.clock()
    local lastTime = start
    function SECTION(...)
        local now = os.clock()
		local line = debug.getinfo(2).currentline
		local func_name = debug.getinfo(2).name or "__main__"
        print(("[%6.3f/%6.3f] %s:%d"):format(now-start, now-lastTime, func_name, line), ...)
        lastTime = now
    end
end

do
    local cache = setmetatable({}, {__mode="k"})
    function getExtent(file, fromRow, fromCol, toRow, toCol)
        if not file then
            DBG(file, fromRow, fromCol, toRow, toCol)
            return ''
        end
        if toRow - fromRow > 3 then
            return ('%s: %d:%d - %d:%d'):format(file, fromRow, fromCol, toRow, toCol)
        end
        if not cache[file] then
            local f = assert(io.open(file))
            local t, n = {}, 0
            for l in f:lines() do
                n = n + 1
                t[n] = l
            end
            cache[file] = t
        end
        local lines = cache[file]
        if not (lines and lines[fromRow] and lines[toRow]) then
            DBG('!!! Missing lines '..fromRow..'-'..toRow..' in file '..file)
            return ''
        end
        if fromRow == toRow then
            return lines[fromRow]:sub(fromCol, toCol-1)
        else
            local res = {}
            for i=fromRow, toRow do
                if i==fromRow then
                    res[#res+1] = lines[i]:sub(fromCol)
                elseif i==toRow then
                    res[#res+1] = lines[i]:sub(1,toCol-1)
                else
                    res[#res+1] = lines[i]
                end
            end
            return table.concat(res, '\n')
        end
    end
end


SECTION "Start"
SECTION 'Creating index'
local index = clang.createIndex(false, true)
SECTION 'Creating index ...'

SECTION 'Creating translation unit'
 local tu = assert(index:parse(arg))
SECTION 'Creating translation unit ...'


local function trim(s)
 local from = s:match"^%s*()"
 local res = from > #s and "" or s:match(".*%S", from)
 return (res:gsub('&', '&amp;'):gsub('<', '&lt;'):gsub('>', '&gt;'):gsub('"', '&quot;'))
end

local function xprint(fmt, ...)
	local line = debug.getinfo(2).currentline
	local func_name = debug.getinfo(2).name or "__main__"
 	local args = {...}
	local i = 1
	for p in fmt:gmatch('%%.') do
		if p == '%s' and type(args[i]) ~= 'string' then
			args[i] = tostring(args[i])
		end
		i = i + 1
	end

	fmt = "%s():%d: "..fmt
	--print( fmt:format( func_name, line, unpack(args)) )
end

local function sprint(fmt, ...)
	local line = debug.getinfo(2).currentline
	local func_name = debug.getinfo(2).name or "__main__"
 	local args = {...}
	local i = 1
	for p in fmt:gmatch('%%.') do
		if p == '%s' and type(args[i]) ~= 'string' then
			args[i] = tostring(args[i])
		end
		i = i + 1
	end

	fmt = "%s():%d: "..fmt
	return( fmt:format( func_name, line, unpack(args)) )
end

local function visitAST2(cur)
    local tag = cur:kind()
    local name = trim(cur:name())
    local attr = ' name="' .. name .. '"'
    local dname = trim(cur:displayName())
    if dname ~= name then
        attr = attr .. ' display="' .. dname .. '"'
    end
    attr = attr ..' text="' .. trim(getExtent(cur:location())) .. '"' 
	if tag == "ParmDecl" or tag == "TypeRef" then
		return
	end
    local children = cur:children()
    if #children == 0 then
		if tag == "FunctionDecl" then
			local result_type = cur:resultType()
			xprint("\t tag=%s, %s, return=%s", tag, attr, tostring(result_type:name()))
		else
			xprint("\t tag=%s, %s", tag, attr)
		end
    else
		local result_type = cur:resultType()
		local func_type
		if tag == "FunctionDecl" then
			result_type = result_type:canonical()
			func_type = cur:type()
		end
		xprint("\t tag=%s, %s, return=%s, type=%s"
		, tag, attr 
		, tostring(result_type and result_type:name())
		, tostring(func_type)
		)
		for _,c in ipairs(children) do
			visitAST(c)
		end
		--xprint("\t --tag=%s, name=%s",  tag, name)
	end
end
local __func__decls = {}
local primitive_type_list = { 
	"Void", 
	"Int", "UInt", 
	"Long" , "ULong", 
	"Short", "UShort",
	"LongLong", "ULongLong",
	"Char_S", "UChar" 
}

local primitive_type_map
local function isPrimitiveType(_type)
	if not primitive_type_map then
		primitive_type_map = {}
		for _, xtype in ipairs(primitive_type_list) do
			primitive_type_map[ xtype ] = true
		end
	end
	return primitive_type_map [tostring(_type)] or false
end

local function toType(xtype)
	if tostring(xtype) == "Pointer" then
		return xtype
	end
	return xtype:canonical()
end

local last_func
local errmsg = ""
local function visitAST(cur)
	local kind = cur:kind()
	if kind == "FunctionDecl" then
		local f = {}
		f.func_name = trim(cur:name())
		f.func_dname = trim(cur:displayName())
		f.func_fname, f.func_line = cur:location()
		f.return_t = assert(cur:resultType())

		f.return_t = toType(f.return_t)
		xprint(" func=%s, dname=%s, loc=%s:%d, return_t=%s, \n\t\t =%s", 
		f.func_name, f.func_dname, f.func_fname, f.func_line,  f.return_t
		, isPrimitiveType(f.return_t))
		__func__decls[f.func_name] = f
		last_func = f.func_name
	elseif kind == "CallExpr" then
		local func_name = trim(cur:name())
		if func_name == "gc_root_new"  or func_name == "gc_root_new_with_complex_return" then
			assert(last_func)
			local f = __func__decls[last_func]
			if func_name == "gc_root_new" then
				if (not isPrimitiveType(f.return_t)) then
					local msg = sprint (" error: call gc_root_new() within %s%d:%s(), return_t=%s, p=%s\n",
					f.func_fname, f.func_line, f.func_name, f.return_t
					, isPrimitiveType(f.return_t))
					errmsg = errmsg .. msg
				end
			elseif func_name == "gc_root_new_with_complex_return" then
				assert(isPrimitiveType(f.return_t) == false)
				if (isPrimitiveType(f.return_t)) then
					xprint (" error: call gc_root_new_with_complex_return() within %s%d:%s(), return_t=%s, p=%s",
					f.func_fname, f.func_line, f.func_name, f.return_t
					, isPrimitiveType(f.return_t))
					--os.exit(1)
				end
			end
		end
	end

	local children = cur:children()
	if #children > 0 then
		for _,c in ipairs(children) do
			visitAST(c)
		end
	end
end

do
	local diagnostics = tu:diagnostics()
	if diagnostics and #diagnostics > 0 then
		for _,v in ipairs(diagnostics) do
			xprint("category=%s, text=%s", v.category, v.text)
		end
		os.exit(0)
	end
end

SECTION "visit AST"
visitAST(tu:cursor())
SECTION "visit AST ..."
if errmsg ~= "" then
	print(errmsg)
	os.exit(1)
end
SECTION "all finish"
os.exit(0)
