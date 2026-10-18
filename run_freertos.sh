#!/bin/bash
# 编译并启动 FreeRTOS 版模拟器。用法: ./run_freertos.sh [--speed 倍数]

cd "$(dirname "$0")"

./build_simulator.sh > /tmp/game_simulator_build.log 2>&1
if [ $? -ne 0 ]; then
    cat /tmp/game_simulator_build.log
    echo "编译失败，模拟器未启动"
    exit 1
fi

echo "启动 FreeRTOS 模拟器..."
./build_simulator/game_simulator_freertos "$@"
