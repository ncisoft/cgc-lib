#!/bin/sh
pushd()
{
  saved_dir=$(pwd)
}

popd()
{
  cd $saved_dir
}

pushd

find /mnt/nfs/cgc-lib/ -regex  ".*\.\(h\|hpp\|c\|cc\)$" \
  |egrep "(src|include|test)/"  \
  | awk '{print $0, $0}'|sed -e "s/ \/mnt\/nfs\/cgc-lib/ \./" \
  |xargs  -n 2 --verbose cp
popd
