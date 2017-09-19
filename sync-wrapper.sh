#!/bin/sh

find wrapper -name "*.*"  \
  | awk '{print $0, $0}'    \
  | sed -e "s/ wrapper/ contrib/" \
  | xargs  -n 2 --verbose cp
