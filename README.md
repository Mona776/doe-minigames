# Doe 小游戏原型与交付工程

包含拔河、贪吃腊肠（三关）、Sausage Snake（无尽）、接网球的原型及 Mac / FreeRTOS 模拟器、Pico 固件适配。

本仓库是现有工程的交付副本，未改动原项目的游戏逻辑、画面和启动方式。素材属于原型资产，正式制作要求见 PRD；此仓库不授予第三方素材的额外授权。

## 从哪里开始

- 想体验：按下面步骤启动 Mac 模拟器。
- 只看 Sausage Snake 无尽版：直接打开 [无尽版交付目录](handoff/sausage-snake/)。其中包含游戏代码、UI、依赖参考和 PRD，不含模拟器；可自行评估哪些代码能用于主项目。
- 想了解无尽版的依赖和接口：阅读 [接入说明](docs/SAUSAGE_INTEGRATION.md)，或从 [Release 下载交付包](https://github.com/Mona776/doe-minigames/releases/tag/sausage-snake-v1.2-handoff)。
- 想了解代码边界：阅读 [目录与依赖](docs/PROJECT_GUIDE.md)。
- 最新无尽版产品需求：[PRD V1.4](设计交付/SausageSnake-无尽版PRD/SausageSnake_无尽版_PRD_V1.4.md)，与[飞书当前文档](https://vcnz89wpi7j4.feishu.cn/wiki/JKSCwKYQEikpt3kNUgScSt84ndh)同步。旧版本与效果稿仅供历史参考。

V1.2 Release 保留为当时的交付快照；主分支及 `handoff/sausage-snake/PRD.md` 已更新为 V1.4 文档，游戏规则和源码不变。

## 目录里都是什么

本仓库保留完整原型工程，所以同时包含游戏、界面、模拟器、真机适配、第三方库及设计文档。目录较多不代表都需要接入主项目。

### 自研代码与资源

- [`core/`](core/)：游戏规则、奖励及存档等逻辑。各小游戏的纯 C 规则在 `core/games/`，不依赖 LVGL、SDL 或硬件。
- [`ui/`](ui/)：游戏页面、绘制、动画、音效控制，以及公共页面管理、主题和最高纪录缓存。
- [`input/`](input/)：按键消抖、按下沿、长按和输入事件处理。
- [`hal/`](hal/)：时钟、音频、按键、文件系统和系统临界区等平台接口声明，具体实现由平台层提供。
- [`platform/`](platform/)：`mac/` 为电脑模拟器适配，`freertos/` 为 Mac 上的 FreeRTOS 任务版模拟器，`pico/` 为 Pico 真机驱动和任务入口。Mac FreeRTOS 模拟器不等同于真机固件。
- [`assets/`](assets/)：生成的字体等资源。部分角色和图标以 C 数据放在 `ui/pages/*_art.c`，不是全部都在 assets 中。
- [`tests/`](tests/)：各游戏规则和输入层的自动测试。

### 第三方库和制作工具

- [`lvgl/`](lvgl/)：界面绘图库源码快照，约200 MB，是仓库体积最大的部分；里面还带有该库的示例、测试和工具，不是我们写的小游戏代码。
- [`FreeRTOS-Kernel/`](https://github.com/raspberrypi/FreeRTOS-Kernel/tree/4f7299d6ea746b27a9dd19e87af568e34bd65b15)：固件使用的任务调度内核和 Pico 移植。
- [`pico-sdk/`](https://github.com/raspberrypi/pico-sdk/tree/079c6f39023649b154152db30f1d781e884879bc)：Pico 芯片开发库。
- [`littlefs/`](https://github.com/littlefs-project/littlefs/tree/6cb4e86540eca0d9ba62500a298385c9d863c8be)：固件文件系统。
- [`tools/`](tools/)：字体转换工具的依赖配置、字体源文件及字体许可。

上面三个固件依赖以 Git submodule 固定版本。直接下载源码 ZIP 不会附带它们的内容，初始化方法见下方“Pico 固件”。Mac 模拟器的 FreeRTOS V11.2.0 由 CMake 单独获取。

### 文档和交付副本

- [`docs/`](docs/)：工程结构、无尽版接入说明和本次交付验证记录。
- [`设计交付/`](设计交付/)：PRD、效果图及历史设计稿，约40 MB；历史稿不代表当前最终需求。
- [`handoff/`](handoff/)：整理好的交付目录、ZIP 包、依赖初始化脚本及规则测试脚本。
- [`handoff/sausage-snake/`](handoff/sausage-snake/)：无尽版代码的独立交付副本。`src/core/games/sausage.*` 是规则，`src/ui/pages/page_sausage*.c` 是页面；其他文件含必要的公共组件参考。文件归属和校验值见 `manifest.json`。

**handoff 与主工程中的部分源码重复，是为了方便单独交付，不是第二套玩法。** 原型工程维护在根目录的 `core/`、`ui/` 等目录中；后续更新游戏后，需要同步刷新交付副本和 ZIP。小包不包含模拟器，也不是免适配插件。

### 根目录的脚本和配置

- `build_simulator.sh` / `run_simulator.sh` / `run_freertos.sh`：编译和运行两种 Mac 模拟器。
- `build_firmware.sh` / `flash.sh`：编译和烧录 Pico 固件。
- `gen_fonts.sh`：根据界面文字生成像素字库。
- `run_tests.sh`：原工程的测试脚本；交付验证推荐使用 `handoff/test_rules.sh`，无需原有构建缓存。
- `CMakeLists.txt`：组织源文件、第三方依赖及编译目标。
- `lv_conf.h` / `FreeRTOSConfig.h`：界面库和任务内核配置。
- `.gitmodules`：固件依赖地址；`.gitignore`：排除本地构建和运行文件。

运行后本地可能出现 `build_simulator/`、`build_firmware/`、`sim_storage/`、`screenshots/` 等目录，分别存放编译产物、存档和截图；这些不提交到仓库。

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
