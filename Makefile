.PHONY : build help build-v init build-clean build-clang build-gcc rebuild release clean clean-fake
.PHONY : run gdb xinit ldd  clang-test func-test
.PHONY : compile compile-v  tags lua-test clang-test uuid check_gc_root_new
.PHONY : xinit snippet

git-dir := $(shell git rev-parse --show-toplevel | tr -d '\n')

build:  snippet compile tags
	#	cd build && ninja -v && cd .. && ctags src/*.cc include/*.h*

build-v: compile-v tags

compile: xinit
		cd build && ninja -v && cd ..
	#	cd ${git-dir}

xinit:
	test -f .xinit_done || (\
		echo "xinit ..." &&  \
		mkdir -p build && \
		./bin/wrapper-init.sh && \
		./bin/wrapper-sync.sh && \
		cd build && \
		CC=gcc CXX=g++ meson -Db_lundef=false .. && \
		cd .. && \
		touch .xinit_done )

compile-v:
	cd build && ninja -v && cd .. && pwd

snippet:
	./bin/translate.lua sync.sh tt include/snippets/tt.h

tags:
	ctags  src/*.[hc] test/*.[hc] include/*.h*

uuid:
	@echo generate uuid ...
	@cat /proc/sys/kernel/random/uuid | sed 's/-/_/g' | \
		awk '{print "UUID_",toupper($$0)}' |  sed 's/ //g'

test:
	cd build && ninja test && pwd && cd .. && ctags src/*.c include/*.h*

rebuild:  clean-fake
	cd build && ninja -v && pwd && cd .. && ctags src/*.c include/*.h*

build-clean:
	rm -f build/*.* build/.ninja_* tags && \
		rm -rf build/menson-logs build/meson-private build/src build/test

build-clang:  build-clean xinit
	./bin/wrapper-init.sh && \
		./bin/wrapper-sync.sh && \
		cd build && \
		CC=clang CXX=clang++ meson .. && cd ..
	cd ${git-dir} && \
		cd build && ninja -v && pwd && cd .. && ctags src/*.c include/*.h*

build-gcc: build-clean xinit
	cd build &&\
		CC=gcc CXX=g++ meson .. && cd ..
	cd build && ninja -v && pwd && cd .. && ctags src/*.c include/*.h*

init: build-clang

release:
	rm -rf build/* tags && cd build && meson --buildtype=release .. && cd ..
	cd build && ninja -v && pwd && cd .. && ctags src/*.c include/*.h*

help:
	@echo "make [ build | build-v | init | build-clang | build-gcc | rebuild | clang-test | "
	@echo "       release |  clean | clean-fake | run | gdb | ldd | check_gc_root_new | help ]\n"

clean:
	true &&                                \
		rm -rf build/* tags .xinit_done && \
		rm -rf ./contrib/libcork &&        \
		rm -rf ./contrib/libev &&          \
		rm -rf ./contrib/micoro &&         \
		rm -rf ./.xopt


clean-fake:
	rm -rf build/src/* build/test/* tags && cd build &&  ninja -t clean

run: build
	#/usr/bin/clear
	@echo ""
	./build/test/gc-test

lua-test: build
	#/usr/bin/clear
	@echo ""
	./build/test/lua-test

clang-test: build
	#/usr/bin/clear
	@echo ""
	./build/test/clang-test

gdb: build
	clear
	gdb ./build/src/test2

ldd: build
	ldd ./build/src/test2
	@echo ""
	ldd ./build/src/clang-test

func-test: build
	./build/src/func-test

check_gc_root_new: build
	lua51 ./src/check_gc_root_new.lua  ./test/gc_test01.c -I./include/ -I/usr/lib/llvm-3.8/lib/clang/3.8.1/include/
	@echo lua51 ./src/check_gc_root_new.lua  ./test/gc_test01.c -I./include/ -I/usr/lib/llvm-3.8/lib/clang/3.8.1/include/

