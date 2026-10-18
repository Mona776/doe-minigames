#!/bin/bash
# Pico 固件编译脚本

cd "$(dirname "$0")"

ARM_TOOLCHAIN_BIN=$(ls -d /Applications/ArmGNUToolchain/*/arm-none-eabi/bin 2>/dev/null | tail -n 1)
if [ -n "$ARM_TOOLCHAIN_BIN" ]; then
    export PATH="$ARM_TOOLCHAIN_BIN:$PATH"
fi

if ! command -v arm-none-eabi-gcc >/dev/null; then
    echo "ARM 工具链未安装"
    exit 1
fi

if [ ! -d "pico-sdk" ] || [ ! -d "FreeRTOS-Kernel" ] || [ ! -d "littlefs" ]; then
    echo "缺少 pico-sdk / FreeRTOS-Kernel / littlefs（从电子宠物工程做符号链接）"
    exit 1
fi

if [ -x "./gen_fonts.sh" ]; then
    ./gen_fonts.sh || { echo "字库生成失败"; exit 1; }
fi

BUILD_DIR="build_firmware"
if [ "$1" == "--clean" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "配置 CMake..."
cmake .. -DBUILD_SIMULATOR=OFF
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

echo "编译成功: build_firmware/game_firmware.uf2"
ls -lh game_firmware.uf2
