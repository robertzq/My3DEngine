#!/usr/bin/env bash
cd build
cmake ..  # 为了保险，加上这一行
make && cd .. && ./build/MyEngine