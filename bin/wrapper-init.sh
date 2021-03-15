#!/bin/sh

pushd()
{
  saved_dir=$(pwd)
}

popd()
{
  cd $saved_dir
}

build_submodule()
{
	submodule_dir="$1"
	cd $git_dir || exit 1
	cd "contrib/$submodule_dir" || exit 1
	make -f Makefile.meson
	cd $git_dir || exit 1
}

git_dir=$(git rev-parse --show-toplevel | tr -d '\n')
pushd

cd $git_dir
mkdir -p build
mkdir -p build/libs

./bin/wrapper-sync.sh

build_submodule "libcork"
build_submodule "libev"
build_submodule "logger"

cd $git_dir
find ./contrib/ -name "*.a" -exec cp --verbose --update {} build/libs/ \;

set -ex
mkdir -p .xopt/include && \
  cp contrib/logger/logger.h .xopt/include/

popd
