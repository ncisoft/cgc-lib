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

./bin/sync-wrapper.sh 

build_submodule "libcork"
build_submodule "libev"

cd $git_dir
find ./contrib/ -name "*.a" -exec cp --verbose --update {} build/libs/ \;

popd
