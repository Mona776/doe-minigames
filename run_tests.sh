#!/bin/bash
# 编译并运行纯 C 单元测试（在模拟器构建目录里）
# 用法: ./run_tests.sh

set -e
cd "$(dirname "$0")"

BUILD_DIR="build_simulator"
mkdir -p "$BUILD_DIR"
(cd "$BUILD_DIR" && cmake .. > /dev/null && make test_input test_tug test_snake test_tennis test_sausage 2>&1 | grep -E "error|warning" || true)

"./$BUILD_DIR/test_input"
echo ""
"./$BUILD_DIR/test_tug"
echo ""
"./$BUILD_DIR/test_snake"
echo ""
"./$BUILD_DIR/test_tennis"
echo ""
"./$BUILD_DIR/test_sausage"
