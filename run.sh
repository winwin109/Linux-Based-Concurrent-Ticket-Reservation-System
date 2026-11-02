#!/usr/bin/bash

make clean

make

if [ -f "server" ]
then
    ./server
else
    echo "没有生成可执行程序"
fi
