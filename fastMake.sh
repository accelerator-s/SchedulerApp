#!/bin/bash

# 默认路径为当前目录的 CMakeLists.txt
cmake_file="CMakeLists.txt"

# 使用 grep 和 sed 提取第一个 add_executable 的目标名
target_name=$(grep -Eo 'add_executable\([^)]+\)' "$cmake_file" | head -n 1 | sed -E 's/add_executable\(\s*([^ ]+).*/\1/')

# 输出结果
if [ -n "$target_name" ]; then
    echo "项目名称：$target_name"
else
    echo "未在 CMakeLists.txt 中找到 add_executable 语句"
    exit 1
fi

printf "需要make什么版本？(输入编号)\n1. Release\n2. Debug\n"
read -r num

if [ ! -d "build" ]; then
    mkdir build
fi
cd build || exit

# 清理之前的构建文件
make clean

# 选择了 Release
if [ "$num" = "1" ]; then

    #清屏
    clear

    printf "编译信息：\n\n"

    cmake -DCMAKE_BUILD_TYPE=Release ..
    if [ $? -ne 0 ]; then
        echo "CMake配置失败，请检查错误信息。"
        exit 1
    fi

    # 编译项目
    make

if [ $? -eq 0 ]; then
    printf "\n编译成功！\n"
else
    printf "\n编译失败！\n"
    exit 1
fi

cd ../output/Release || exit
printf "\n尝试运行编译后的程序...\n\n"
./"$target_name"
exit

#选择了 Debug
elif [ "$num" = "2" ]; then

    #清屏
    clear

    printf "编译信息：\n\n"

    cmake -DCMAKE_BUILD_TYPE=Debug ..
    if [ $? -ne 0 ]; then
        echo "CMake配置失败，请检查错误信息。"
        exit 1
    fi
    
    # 编译项目
    make

if [ $? -eq 0 ]; then
    printf "\n编译成功！\n"
else
    printf "\n编译失败！\n"
    exit 1
fi

cd ../output/Debug || exit
printf "\n尝试运行编译后的程序...\n\n"
./"$target_name"
exit

else
    echo "输入错误，脚本退出..."
    exit 1
fi





