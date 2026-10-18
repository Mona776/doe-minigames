/**
 * Pico 2W 片上 4MB Flash 分区
 *
 *   0x000000 - 0x17FFFF  固件（最大 1.5MB）
 *   0x180000 - 0x37FFFF  素材分区（2MB，LittleFS 只读，由 tools/gen_assets.py 打包、flash.sh 一并烧录）
 *   0x380000 - 0x3FFFFF  数据分区（512KB，LittleFS 读写，存档等；重刷固件和素材都不会动它）
 *
 * tools/ 下的 Python 脚本会解析本文件里的 #define，改分区只需改这里
 */

#ifndef FLASH_LAYOUT_H
#define FLASH_LAYOUT_H

#define FLASH_TOTAL_SIZE     (4 * 1024 * 1024)

#define FLASH_FIRMWARE_MAX   (1536 * 1024)

#define FLASH_ASSETS_OFFSET  (1536 * 1024)
#define FLASH_ASSETS_SIZE    (2048 * 1024)

#define FLASH_DATA_OFFSET    (3584 * 1024)
#define FLASH_DATA_SIZE      (512 * 1024)

// LittleFS 参数，素材镜像打包时必须一致
#define FLASH_LFS_BLOCK_SIZE       4096
#define FLASH_LFS_PROG_SIZE        256
#define FLASH_LFS_READ_SIZE        256
#define FLASH_LFS_CACHE_SIZE       256
#define FLASH_LFS_LOOKAHEAD_SIZE   16
#define FLASH_LFS_BLOCK_CYCLES     500

#endif
