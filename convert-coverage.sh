#!/bin/bash

IFS=' ' read -r -a array <<< `find . -name *.gcno`

for item in "${array[@]}"
do
    dir=`dirname ${item}`
    pushd ${dir}
    gcov -a *.o
    popd
done
