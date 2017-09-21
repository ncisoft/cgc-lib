.PHONY : build help build-v init build-clang build-gcc rebuild release clean clean-fake 
.PHONY : run gdb xinit ldd  clang-test func-test
.PHONY : compile compile-v  tags lua-test clang-test uuid

git-dir := $(shell git rev-parse --show-toplevel | tr -d '\n')

build: xinit compile tags
#	cd build && ninja -v && cd .. && ctags src/*.cc include/*.h*

build-v: compile-v tags 

compile: 
	cd build && ninja -v && cd .. 
#	cd ${git-dir}

xinit:
	mkdir -p build
	./bin/wrapper-init.sh
	./bin/sync-wrapper.sh

compile-v:
	cd build && ninja -v && cd .. && pwd

tags: 
	ctags  src/*.c test/*.c include/*.h*

uuid:
	@echo generate uuid ...
	@cat /proc/sys/kernel/random/uuid | sed 's/-/_/g' | \
	awk '{print "UUID_",toupper($$0)}' |  sed 's/ //g'

test:
	cd build && ninja test && pwd && cd .. && anjuta-tags src/*.c include/*.h*

rebuild:  clean-fake
	cd build && ninja -v && pwd && cd .. && anjuta-tags src/*.c include/*.h*

build-clang: 
	rm -rf build/* tags && cd build && CC=clang CXX=clang++ meson .. && cd ..
	cd build && ninja -v && pwd && cd .. && anjuta-tags src/*.c include/*.h*

build-gcc: 
	rm -rf build/* tags && cd build && CC=gcc CXX=g++ meson .. && cd ..
	cd build && ninja -v && pwd && cd .. && anjuta-tags src/*.c include/*.h*

init: build-clang

release:
	rm -rf build/* tags && cd build && meson --buildtype=release .. && cd ..
	cd build && ninja -v && pwd && cd .. && anjuta-tags src/*.c include/*.h*

help:
	@echo "make [ build | build-v | init | build-clang | build-gcc | rebuild | clang-test | "
	@echo "       release |  clean | clean-fake | run | gdb | ldd | help ]\n"

clean:
	rm -rf build/* tags && cd build && meson ..

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

