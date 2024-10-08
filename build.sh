#!/bin/bash

# 检查build目录是否存在，如果不存在则创建
if [ ! -d "build" ]; then
    echo "Create build directory..."
    mkdir build
fi

# 进入build目录
cd build

# 运行CMake生成Makefile
echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 编译项目
echo "Compiling project..."
make -j8

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "Compilation successful!."
else
    echo "Compilation failed."
fi

# 返回到原始目录
cd ..
