test_source = files(
    'gc_test.c')


executable('t.out', 't.c',
           include_directories : [inc,luaext_inc],
           dependencies : [ gc_dep ],
           )


executable('stack-test', 'stack-test.c',
           include_directories : [inc,luaext_inc]
           )

executable('backstrace-test', 'backstrace.c',
           include_directories : [inc,luaext_inc],
           dependencies : [gc_dep, lua, luaext_dep, jemalloc_dep  ])

executable('gc-test', test_source,
		include_directories : [inc,luaext_inc],
		link_args :  ['-rdynamic'],
		dependencies : [gc_dep, lua, luaext_dep ])

luatest_source = files(
    'lua-test.c')


executable('lua-test', luatest_source,
		include_directories : inc,
		link_args :  ['-rdynamic'],
		dependencies : [gc_dep, lua, luaext_dep ])

clangtest_source = files(
    'clang-test.c')


executable('clang-test', clangtest_source,
		include_directories : inc,
		link_args :  ['-rdynamic'],
		dependencies : [gc_dep, lua, luaext_dep ])

setjumptest_source = files(
    'setjump-test.c')


executable('setjump-test', 'setjump-test.c',
		include_directories : inc,
		link_args :  ['-rdynamic'],
		dependencies : [gc_dep, lua, luaext_dep, libev_dep, cork_dep ])




