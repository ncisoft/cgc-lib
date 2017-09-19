lib_source = files(
    'luaext.c')

luaext_inc = include_directories('.')

luaext_lib = static_library('luaext', 'luaext.c'
		, include_directories : inc
		, dependencies : [lua]
		)

luaext_dep = declare_dependency(link_with : luaext_lib, include_directories : luaext_inc)

