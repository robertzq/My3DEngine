#!/usr/bin/env bash
# 确保 build 目录存在
if [ ! -d "build" ]; then
  mkdir build
fi
cd build
cmake ..  # 为了保险，加上这一行
make && cd .. && ./build/MyEngine