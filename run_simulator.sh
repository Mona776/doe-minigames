#!/bin/bash
# 先增量编译再启动。用法: ./run_simulator.sh [--speed 倍数]

cd "$(dirname "$0")"

./build_simulator.sh > /tmp/game_simulator_build.log 2>&1
if [ $? -ne 0 ]; then
    cat /tmp/game_simulator_build.log
    echo "编译失败，模拟器未启动"
    exit 1
fi

echo "启动模拟器..."
./build_simulator/game_simulator "$@"
