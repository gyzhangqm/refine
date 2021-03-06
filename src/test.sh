#!/usr/bin/env bash

CC=gcc
CFLAGS="-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused"

if [ -z "$1" ]
then
  tests=`ls -1 *_test.c`
else
  tests=$*
fi

for test in $tests;
do
  root=${test%_test.c}
  echo \$ $0 $root

  dependencies=''
  if [ -a ${root}.h ]; then
    for dep_header in `grep '#include' ${root}.h | grep '"ref_'`
    do
      source_with_leading_quote=${dep_header%.h\"}.c
      source=${source_with_leading_quote#\"}
      if [ -a ${source} ]; then
        if ! echo ${dependencies} | grep -q "${source}" ; then
          dependencies=${dependencies}' '${source}
        fi
      fi
    done
  fi
  for dep_header in `grep '#include' ${root}_test.c | grep '"ref_'`
  do
    source_with_leading_quote=${dep_header%.h\"}.c
    source=${source_with_leading_quote#\"}
    if [ -a ${source} ]; then
      if ! echo ${dependencies} | grep -q "${source}" ; then
        dependencies=${dependencies}' '${source}
      fi
    fi
  done

  compile="${CC} ${CFLAGS} -o ${root}_test ${dependencies} ${root}_test.c -lm "
  (eval ${compile} && eval valgrind --quiet --error-exitcode=1 --leak-check=full ./${root}_test) || \
      ( echo ${compile} && echo FAIL: to re-run, \$ $0 ${root} ; exit 1 ) ||  exit 1
done

