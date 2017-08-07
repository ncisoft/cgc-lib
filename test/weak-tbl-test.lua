tbl = {}
mt =  {__mode = "k"}
setmetatable(tbl, mt)
key = {}
tbl[key] = 1
tbl["fd"] = 1
key = nil
collectgarbage()
for k,v in pairs(tbl) do
	print(k)
end
