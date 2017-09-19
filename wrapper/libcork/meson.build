lib_source = files(
		'./src/libcork/cli/commands.c',
		'./src/libcork/core/allocator.c',
		'./src/libcork/core/error.c',
		'./src/libcork/core/gc.c',
		'./src/libcork/core/hash.c',
		'./src/libcork/core/ip-address.c',
		'./src/libcork/core/mempool.c',
		'./src/libcork/core/timestamp.c',
		'./src/libcork/core/u128.c',
		'./src/libcork/core/version.c',
		'./src/libcork/ds/array.c',
		'./src/libcork/ds/bitset.c',
		'./src/libcork/ds/buffer.c',
		'./src/libcork/ds/dllist.c',
		'./src/libcork/ds/file-stream.c',
		'./src/libcork/ds/hash-table.c',
		'./src/libcork/ds/managed-buffer.c',
		'./src/libcork/ds/ring-buffer.c',
		'./src/libcork/ds/slice.c',
		'./src/libcork/posix/directory-walker.c',
		'./src/libcork/posix/env.c',
		'./src/libcork/posix/exec.c',
		'./src/libcork/posix/files.c',
		'./src/libcork/posix/process.c',
		'./src/libcork/posix/subprocess.c',
		'./src/libcork/pthreads/thread.c',
		)

		cork_c_args= ['-DCORK_API=CORK_EXPORT', '-D_POSIX_SOURCE', '-D_BSD_SOURCE']
		cork_inc = include_directories('include')
		cork_lib = static_library('cork' 
				, lib_source 
				, c_args :cork_c_args
				, include_directories : cork_inc)

		cork_dep = declare_dependency(link_with : cork_lib,
				include_directories : cork_inc)
