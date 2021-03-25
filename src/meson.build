libs_source = files(
    'gc_lua.c')

gc_inc = include_directories('.')
gc_lib =  static_library('cgc',
		['gc_lua.c', 'coroutine.c'],
		include_directories : inc, dependencies:[lua, luaext_dep, cork_dep])

gc_dep = declare_dependency(
		link_with :  gc_lib,
		include_directories :  gc_inc
		)


