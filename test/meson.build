test_source = files(
    'gc_test.c')


executable('gc-test', test_source,
		include_directories : [inc,luaext_inc],
		link_args :  ['-rdynamic'],
		dependencies : [gc_dep, lua, luaext_dep, jemalloc_dep  ])

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




