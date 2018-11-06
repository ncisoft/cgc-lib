git submodule update --init --recursive

lua51 src/check_gc_root_new.lua /tmp/lua-test.c  -I./include/  -I./contrib/lua51-ext/ -I/usr/lib/llvm-3.8/lib/clang/3.8.1/include/ -I/usr/include/lua5.1
