#!/bin/bash
# Mac 模拟器编译脚本

cd "$(dirname "$0")"

if ! brew list sdl2 &>/dev/null; then
    echo "SDL2 未安装，正在安装..."
    brew install sdl2
fi

./gen_fonts.sh || { echo "字库生成失败"; exit 1; }

BUILD_DIR="build_simulator"
if [ "$1" == "--clean" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "配置 CMake..."
cmake .. -DBUILD_SIMULATOR=ON
if [ $? -ne 0 ]; then
    echo "CMake 配置失败"
    exit 1
fi

echo "开始编译..."
make -j"$(sysctl -n hw.ncpu)"
if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

echo "编译成功: build_simulator/game_simulator"
echo "           build_simulator/game_simulator_freertos"
