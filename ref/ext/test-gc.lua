obj = {}
function fgc(o)
	print(string.format("[GC] collected %s", o))
end
function log(fmt, ...)
	local args={fmt, ...}
	local args2 = {}
	local unpack = unpack or table.unpack
	for i,v in ipairs(args) do
		if (type(v) == "string") then
			args2[i] = v
		else
			args2[i] = tostring(v)
		end
	end
	print(string.format(unpack(args2, 1)))
end

mt={__gc=fgc}
debug.setmetatable(obj,mt)
tbl={}
mt={__mode="kv"}
--setmetatable(tbl,mt)
log("obj is %s", obj)
tbl[obj] = 1
tbl.fd2 = 1
tbl[obj] = nil
--tbl[fd] = nil
obj = nil
collectgarbage ()
print("\nafter collectgarbage, walk throught")
for k,v in pairs(tbl) do
	print(k)
end
print("walk throught -- end\n")
