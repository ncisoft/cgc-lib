project('ansic-gc', 'c', 'cpp', default_options : ['c_std=c99', 'cpp_std=c++11'],
        version: '0.1.0-dev',
        license: 'MIT',
        meson_version: '>=0.36.0')

api_version = '0.1'


add_global_arguments('-Wno-unused-label', language : 'c')
#add_global_arguments('-fcolor-diagnostics, -DCORK_API=CORK_EXPORT', language : 'c')
# add_global_arguments('-Wall', language : 'cpp')
cxx_args = ['-fcolor-diagnostics']
xlink_args = ['-rdynamic','-ldw']

inc = include_directories(['./include'])
jemalloc_dep = dependency('jemalloc', version: '>=3.0')
lua = dependency('lua-5.1', version: '>=0.20')
thread_dep = dependency('threads')

subdir('contrib/lua51-ext')
subdir('contrib/libcork')
subdir('src')
subdir('test')
if get_option('enable_examples')
    subdir('examples')
endif
message( 'here' )
