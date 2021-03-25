#!/bin/sh

tmpc_fname=$(mktemp /tmp/test-XXXXXX.c)
echo  "" > $tmpc_fname
echo  "#include <stdint.h>" >> $tmpc_fname
#echo  "#include <stdbool.h>" >> $tmpc_fname
gcc -E $tmpc_fname |grep typedef
rm $tmpc_fname
