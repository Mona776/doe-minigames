/**
 * 数据分区文件接口（存档、以后下载的内容）
 * Mac: 项目下 sim_storage/data/ 文件夹；Pico: Flash 末尾的 LittleFS 数据分区
 * 素材分区只读，由 LVGL 的文件系统驱动直接访问，不走这里
 * 只能在逻辑任务中调用
 */

#ifndef HAL_FS_H
#define HAL_FS_H

#include <stdbool.h>
#include <stdint.h>

bool hal_fs_init(void);

// 返回读到的字节数，文件不存在或出错返回 -1
int32_t hal_fs_read(const char* path, void* buf, uint32_t size);

// 覆盖写整个文件
bool hal_fs_write(const char* path, const void* data, uint32_t size);

// 目标已存在时原子替换
bool hal_fs_rename(const char* from, const char* to);

bool hal_fs_remove(const char* path);

#endif
