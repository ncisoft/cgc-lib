#!/bin/bash
pushd()
{
  saved_dir=$(pwd)
}

popd()
{
  cd $saved_dir
}

pushd
git_dir=$(git rev-parse --show-toplevel | tr -d '\n')
cd $git_dir
echo "cwd = $(pwd)"

submodule_s=$( git submodule status  contrib/libcork | sed -e "s/^\-.*$/\-/")
if test "$submodule_s" = "-"  
then
	git submodule update --init --recursive
	find wrapper/ -name "*.*" |xargs touch
else
	echo "submodule has been updated"
fi

touch_out=$(find wrapper -name "*.*" 	\
	| xargs --verbose neko  ./bin/cmp-touch.n .phony_target)

if test "x$touch_out" = "x.phony_target" 
then
	echo "sync wrapper was clean"
else
	echo "---- sync start ---- $(pwd)"
	echo "\"cp --update -r wrapper/* contrib/\"" |xargs --verbose sh -c
	echo "----"
fi

popd

