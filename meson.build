project('cpp-gc', 'c', default_options : ['c_std=c99'],
        version: '0.1.0-dev',
        license: 'MIT',
        meson_version: '>=0.36.0')

api_version = '0.1'

# add_global_arguments('-Wall', language : 'cpp')
xcpp_args = ['-DCPPTHING']
xlink_args = ['-rdynamic','-ldw']

inc = include_directories('include')
jemalloc = dependency('jemalloc', version: '>=3.0')
lua = dependency('lua-5.1', version: '>=0.20')
thread_dep = dependency('threads')

subdir('contrib/lua51-ext')
subdir('src')
subdir('test')
if get_option('enable_examples')
    subdir('examples')
endif
