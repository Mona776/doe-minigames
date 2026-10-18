/**
 * Flash 文件系统 - Pico 实现
 * 两个 LittleFS 分区（见 flash_layout.h）：
 *   数据分区：读写，给存档用，挂载失败（首次上电）时自动格式化
 *   素材分区：只读，交给 LVGL 的 littlefs 驱动；块数从镜像的超级块读取
 * 读直接走 XIP 内存映射，写和擦除通过 flash_safe_execute 在关中断状态下执行
 */

#include <stdio.h>
#include <string.h>
#include "hardware/flash.h"
#include "pico/flash.h"
#include "lfs.h"
#include "flash_layout.h"
#include "hal/hal_fs.h"
#include "hal_fs_pico.h"

#define FLASH_OP_TIMEOUT_MS 1000

typedef struct {
    uint32_t offset;
    bool writable;
} Partition;

typedef struct {
    uint32_t offset;
    const void* data;
    uint32_t size;
} FlashOp;

static const Partition s_data_part = {FLASH_DATA_OFFSET, true};
static const Partition s_assets_part = {FLASH_ASSETS_OFFSET, false};

static lfs_t s_data_fs;
static lfs_t s_assets_fs;
static bool s_data_ok;
static bool s_assets_ok;

static int bd_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {
    const Partition* p = c->context;
    memcpy(buffer, (const uint8_t*)XIP_BASE + p->offset + block * c->block_size + off, size);
    return LFS_ERR_OK;
}

static void do_program(void* arg) {
    const FlashOp* op = arg;
    flash_range_program(op->offset, op->data, op->size);
}

static void do_erase(void* arg) {
    const FlashOp* op = arg;
    flash_range_erase(op->offset, op->size);
}

static int bd_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {
    const Partition* p = c->context;
    if (!p->writable) return LFS_ERR_IO;
    FlashOp op = {p->offset + block * c->block_size + off, buffer, size};
    return flash_safe_execute(do_program, &op, FLASH_OP_TIMEOUT_MS) == PICO_OK ? LFS_ERR_OK : LFS_ERR_IO;
}

static int bd_erase(const struct lfs_config* c, lfs_block_t block) {
    const Partition* p = c->context;
    if (!p->writable) return LFS_ERR_IO;
    FlashOp op = {p->offset + block * c->block_size, NULL, c->block_size};
    return flash_safe_execute(do_erase, &op, FLASH_OP_TIMEOUT_MS) == PICO_OK ? LFS_ERR_OK : LFS_ERR_IO;
}

static int bd_sync(const struct lfs_config* c) {
    (void)c;
    return LFS_ERR_OK;
}

#define LFS_COMMON_CONFIG                              \
    .read = bd_read,                                   \
    .prog = bd_prog,                                   \
    .erase = bd_erase,                                 \
    .sync = bd_sync,                                   \
    .read_size = FLASH_LFS_READ_SIZE,                  \
    .prog_size = FLASH_LFS_PROG_SIZE,                  \
    .block_size = FLASH_LFS_BLOCK_SIZE,                \
    .block_cycles = FLASH_LFS_BLOCK_CYCLES,            \
    .cache_size = FLASH_LFS_CACHE_SIZE,                \
    .lookahead_size = FLASH_LFS_LOOKAHEAD_SIZE

static const struct lfs_config s_data_cfg = {
    .context = (void*)&s_data_part,
    .block_count = FLASH_DATA_SIZE / FLASH_LFS_BLOCK_SIZE,
    LFS_COMMON_CONFIG,
};

static const struct lfs_config s_assets_cfg = {
    .context = (void*)&s_assets_part,
    .block_count = 0,
    LFS_COMMON_CONFIG,
};

bool hal_fs_init(void) {
    int err = lfs_mount(&s_data_fs, &s_data_cfg);
    if (err) {
        printf("[FS] 数据分区未格式化（%d），正在格式化...\n", err);
        err = lfs_format(&s_data_fs, &s_data_cfg);
        if (!err) err = lfs_mount(&s_data_fs, &s_data_cfg);
    }
    s_data_ok = err == 0;
    printf("[FS] 数据分区 %s（%uKB）\n", s_data_ok ? "已挂载" : "挂载失败", FLASH_DATA_SIZE / 1024);

    err = lfs_mount(&s_assets_fs, &s_assets_cfg);
    s_assets_ok = err == 0;
    if (s_assets_ok) {
        printf("[FS] 素材分区已挂载（%u/%u 块）\n", (unsigned)lfs_fs_size(&s_assets_fs),
               (unsigned)s_assets_fs.block_count);
    } else {
        printf("[FS] 素材分区挂载失败（%d），请用 flash.sh 烧录带素材的完整固件\n", err);
    }
    return s_data_ok;
}

void hal_fs_pico_print_info(void) {
    if (s_data_ok) {
        struct lfs_info info;
        bool has_save = lfs_stat(&s_data_fs, "pet.sav", &info) == 0;
        printf("[FS] 数据分区: 已用 %d/%u 块，存档 %s", (int)lfs_fs_size(&s_data_fs),
               (unsigned)s_data_cfg.block_count, has_save ? "pet.sav" : "无");
        if (has_save) printf(" %u 字节", (unsigned)info.size);
        printf("\n");
    } else {
        printf("[FS] 数据分区: 未挂载\n");
    }
    if (s_assets_ok) {
        printf("[FS] 素材分区: 镜像 %u 块，已用 %d 块，分区共 %u 块\n", (unsigned)s_assets_fs.block_count,
               (int)lfs_fs_size(&s_assets_fs), FLASH_ASSETS_SIZE / FLASH_LFS_BLOCK_SIZE);
    } else {
        printf("[FS] 素材分区: 未挂载\n");
    }
}

lfs_t* hal_fs_pico_assets(void) {
    return s_assets_ok ? &s_assets_fs : NULL;
}

int32_t hal_fs_read(const char* path, void* buf, uint32_t size) {
    if (!s_data_ok) return -1;
    lfs_file_t f;
    if (lfs_file_open(&s_data_fs, &f, path, LFS_O_RDONLY) < 0) return -1;
    lfs_ssize_t n = lfs_file_read(&s_data_fs, &f, buf, size);
    lfs_file_close(&s_data_fs, &f);
    return n < 0 ? -1 : (int32_t)n;
}

bool hal_fs_write(const char* path, const void* data, uint32_t size) {
    if (!s_data_ok) return false;
    lfs_file_t f;
    if (lfs_file_open(&s_data_fs, &f, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) return false;
    bool ok = lfs_file_write(&s_data_fs, &f, data, size) == (lfs_ssize_t)size;
    ok = lfs_file_close(&s_data_fs, &f) == 0 && ok;
    return ok;
}

bool hal_fs_rename(const char* from, const char* to) {
    return s_data_ok && lfs_rename(&s_data_fs, from, to) == 0;
}

bool hal_fs_remove(const char* path) {
    return s_data_ok && lfs_remove(&s_data_fs, path) == 0;
}
