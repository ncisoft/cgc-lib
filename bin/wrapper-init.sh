#!/bin/sh

pushd()
{
  saved_dir=$(pwd)
}

popd()
{
  cd $saved_dir
}

git_dir=$(git rev-parse --show-toplevel | tr -d '\n')
pushd

cd $git_dir
mkdir -p build
mkdir -p build/libs

submodule_s=$( git submodule status  contrib/libcork | sed -e "s/^\-.*$/\-/")

if test "$submodule_s" = "-"  
then
	git submodule update --init --recursive
        mkdir -p $gitdir/contrib/.libs
else
	echo "submodule has been updated"
        echo mkdir -p $git_dir/contrib/.libs
fi
./bin/sync-wrapper.sh 

cd contrib/libcork
mkdir -p build
cd build
cmake .. || exit 1
make -j3 || exit 1

cd $git_dir
find ./contrib/ -name "*.a" -exec cp --verbose {} build/libs/ \;

popd
