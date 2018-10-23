.PHONY : build help build-v init build-clean build-clang build-gcc rebuild release clean clean-fake 
.PHONY : run gdb xinit ldd  clang-test func-test
.PHONY : compile compile-v  tags lua-test clang-test uuid
.PHONY : xinit

git-dir := $(shell git rev-parse --show-toplevel | tr -d '\n')

build: compile tags
	#	cd build && ninja -v && cd .. && ctags src/*.cc include/*.h*

build-v: compile-v tags 

compile: xinit
	rm -f build/*.* build/.ninja_* tags && \
		rm -rf build/menson-logs build/meson-private build/contrib uild/src build/test && \
		cd build && \
		CC=clang CXX=clang++ meson .. && \
		ninja -v && cd .. 
	#	cd ${git-dir}

xinit:
	test ! -f .xinit_done || \
		mkdir -p build && \
		./bin/wrapper-init.sh && \
		./bin/wrapper-sync.sh && \
		cd build && \
		CC=clang CXX=clang++ meson -Db_lundef=false .. && \
		cd .. && \
		touch .xinit_done

compile-v:
	cd build && ninja -v && cd .. && pwd

tags: 
	ctags  src/*.c test/*.c include/*.h*

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
	@echo "       release |  clean | clean-fake | run | gdb | ldd | help ]\n"

clean:
	rm -rf build/* tags .xinit_done && \
		rm -rf ./contrib/libcork && \
		rm -rf ./contrib/libev && \
		rm -rf ./contrib/micoro


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

