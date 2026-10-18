/**
 * Pico 专用：取得素材分区的 LittleFS 句柄交给 LVGL
 */

#ifndef HAL_FS_PICO_H
#define HAL_FS_PICO_H

#include "lfs.h"

// hal_fs_init 之后调用；素材分区挂载失败返回 NULL
lfs_t* hal_fs_pico_assets(void);

// 打印两个分区的使用情况（fs 命令）
void hal_fs_pico_print_info(void);

#endif
