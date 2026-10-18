# Doe 小游戏原型与交付工程

包含拔河、贪吃腊肠（三关）、Sausage Snake（无尽）、接网球的原型及 Mac / FreeRTOS 模拟器、Pico 固件适配。

本仓库是现有工程的交付副本，未改动原项目的游戏逻辑、画面和启动方式。素材属于原型资产，正式制作要求见 PRD；此仓库不授予第三方素材的额外授权。

## 从哪里开始

- 想体验：按下面步骤启动 Mac 模拟器。
- 想接入 Sausage Snake：阅读 [接入说明](docs/SAUSAGE_INTEGRATION.md)，或下载 Release 中的 `sausage-snake-handoff.zip`。
- 想了解代码边界：阅读 [目录与依赖](docs/PROJECT_GUIDE.md)。
- 最新无尽版产品需求：[PRD V1.2](设计交付/SausageSnake-无尽版PRD/SausageSnake_单键右转_PRD_V1.2.md)。旧版本与效果稿仅供历史参考。

## Mac 模拟器

需要 Xcode Command Line Tools、CMake、SDL2；重新生成字体还需要 Node.js/npm。首次配置需要网络访问 GitHub 拉取固定版本的 FreeRTOS（V11.2.0）。

```bash
brew install cmake sdl2 node
./run_simulator.sh
```

FreeRTOS 任务版：

```bash
./run_freertos.sh
```

键盘 A=选择、S=确定、D=返回，空格=触摸，K=截图，Esc=退出。S 对应设备 B 键。

## 规则测试（无需 SDL、LVGL 或 SDK）

```bash
./handoff/test_rules.sh
```

## Pico 固件

SDK、FreeRTOS Pico 移植及 littlefs 固定到原工程使用的提交。仓库不包含原机的绝对路径符号链接。克隆和 ZIP 下载两种方式均可使用：

```bash
./handoff/setup_firmware_deps.sh
./build_firmware.sh
```

另外需要 ARM GNU Toolchain、CMake，烧录步骤见 `flash.sh`。Pico 构建通过不等于对应屏幕、GPIO、音频已在同事的硬件上验收；先核对 `platform/pico/board_pins.h`。

## 数据与交付范围

源码不包含编译缓存、个人存档、运行日志、node_modules、截图或原电脑外链。运行时会重新生成本地存档目录。保留 LVGL 原始源码及许可证；不修改游戏资产。
