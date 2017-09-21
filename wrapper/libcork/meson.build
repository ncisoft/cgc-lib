# cork_c_args= ['-DCORK_API=CORK_EXPORT', '-D_POSIX_SOURCE', '-D_BSD_SOURCE']
cork_inc = include_directories('include')
#cork_lib = static_library('cork' 
#		, lib_source 
#		, c_args :cork_c_args
#		, include_directories : cork_inc)

cork_dep = declare_dependency(link_args : ['-L./libs/','-lcork']
		, include_directories : cork_inc)
