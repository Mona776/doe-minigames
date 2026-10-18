cmake_minimum_required(VERSION 3.13)

# 两个目标共用：LVGL + 游戏逻辑 + 输入 + UI + 字库
macro(add_game_core)
    set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
    set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    add_subdirectory(lvgl)

    file(GLOB GAME_ASSET_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/generated/*.c)

    add_library(game_core STATIC
        core/game_time.c
        core/game_service.c
        core/games/tug.c
        core/games/snake.c
        core/games/tennis.c
        core/games/sausage.c
        core/save.c
        input/input.c
        ui/ui_app.c
        ui/ui_events.c
        ui/ui_records.c
        ui/ui_sfx.c
        ui/page_mgr.c
        ui/pages/page_home.c
        ui/pages/page_menu.c
        ui/pages/game_art.c
        ui/pages/page_tug.c
        ui/pages/tug_art.c
        ui/pages/page_snake.c
        ui/pages/page_snake_entry.c
        ui/pages/snake_art.c
        ui/pages/snake_ui.c
        ui/pages/sausage_ui.c
        ui/pages/page_tennis.c
        ui/pages/tennis_art.c
        ui/pages/page_sausage.c
        ui/pages/page_sausage_entry.c
        ui/asset.c
        ui/sprite.c
        ui/ui_theme.c
        ui/crt.c
        ui/console.c
        ${GAME_ASSET_SOURCES}
    )
    target_include_directories(game_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(game_core PUBLIC lvgl)
endmacro()

if(DEFINED BUILD_SIMULATOR AND BUILD_SIMULATOR)
    message(STATUS "构建目标: Mac 模拟器")
    project(game_simulator C)
    set(CMAKE_C_STANDARD 11)

    find_package(SDL2 REQUIRED)
    add_compile_definitions(
        PET_SIMULATOR
        PET_SIM_STORAGE_DIR="${CMAKE_SOURCE_DIR}/sim_storage"
    )
    set(CONFIG_LV_USE_SDL ON CACHE BOOL "" FORCE)
    add_game_core()
    target_link_libraries(game_core PUBLIC SDL2::SDL2)

    add_executable(game_simulator
        platform/mac/main.c
        platform/mac/hal_mac.c
        platform/mac/hal_input_mac.c
        platform/mac/hal_fs_mac.c
        platform/mac/hal_audio_mac.c
        platform/mac/app_msg_mac.c
    )
    target_link_libraries(game_simulator game_core SDL2::SDL2)

    include(FetchContent)
    FetchContent_Declare(
        FreeRTOS-Kernel
        GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
        GIT_TAG V11.2.0
        GIT_SHALLOW TRUE
    )
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/platform/freertos)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
    set(FREERTOS_PORT GCC_POSIX CACHE STRING "" FORCE)
    set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
    FetchContent_MakeAvailable(FreeRTOS-Kernel)

    add_executable(game_simulator_freertos
        platform/freertos/main.c
        platform/freertos/app_msg_freertos.c
        platform/freertos/sim_host.c
        platform/freertos/hal_input.c
        platform/mac/hal_mac.c
        platform/mac/hal_fs_mac.c
        platform/mac/hal_audio_mac.c
    )
    target_include_directories(game_simulator_freertos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/freertos)
    target_link_libraries(game_simulator_freertos game_core freertos_kernel SDL2::SDL2)

    add_executable(test_input tests/test_input.c input/input.c)
    target_include_directories(test_input PRIVATE ${CMAKE_SOURCE_DIR})
    add_executable(test_tug tests/test_tug.c core/games/tug.c)
    target_include_directories(test_tug PRIVATE ${CMAKE_SOURCE_DIR})
    add_executable(test_snake tests/test_snake.c core/games/snake.c)
    target_include_directories(test_snake PRIVATE ${CMAKE_SOURCE_DIR})
    add_executable(test_tennis tests/test_tennis.c core/games/tennis.c)
    target_include_directories(test_tennis PRIVATE ${CMAKE_SOURCE_DIR})
    add_executable(test_sausage tests/test_sausage.c core/games/sausage.c)
    target_include_directories(test_sausage PRIVATE ${CMAKE_SOURCE_DIR})

    message(STATUS "输出: game_simulator, game_simulator_freertos")

else()
    message(STATUS "构建目标: Pico 固件")

    set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/pico-sdk")
    set(PICO_BOARD pico2_w CACHE STRING "Board type")
    set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Platform")
    set(FREERTOS_KERNEL_PATH "${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel")

    include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
    include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

    project(game_firmware C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    pico_sdk_init()

    include_directories(${CMAKE_SOURCE_DIR}/littlefs)
    add_library(littlefs STATIC littlefs/lfs.c littlefs/lfs_util.c)
    target_compile_definitions(littlefs PUBLIC LFS_NO_DEBUG)

    add_game_core()
    target_link_libraries(lvgl PUBLIC littlefs)

    add_executable(game_firmware
        platform/pico/main.c
        platform/pico/hal_pico.c
        platform/pico/hal_fs_pico.c
        platform/pico/hal_input_pico.c
        platform/pico/hal_audio_pico.c
        platform/pico/app_msg_pico.c
        platform/pico/st7789.c
        FreeRTOSConfig.h
    )

    target_include_directories(game_firmware PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/platform/pico)

    target_compile_definitions(game_firmware PRIVATE
        CYW43_LWIP=0
        PICO_FLASH_ASSUME_CORE1_SAFE=1
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=2500
    )

    target_link_libraries(game_firmware
        game_core
        littlefs
        pico_stdlib
        pico_flash
        hardware_flash
        hardware_watchdog
        hardware_spi
        hardware_dma
        pico_cyw43_arch_sys_freertos
        FreeRTOS-Kernel-Heap4
    )

    pico_enable_stdio_usb(game_firmware 1)
    pico_enable_stdio_uart(game_firmware 0)
    pico_add_extra_outputs(game_firmware)

    message(STATUS "SDK: ${PICO_SDK_PATH}")
    message(STATUS "输出: game_firmware.uf2")
endif()
