/**
 * 存档读写
 */

#include "save.h"
#include <stdio.h>
#include <string.h>
#include "hal/hal_fs.h"

#define SAVE_PATH     "games.sav"
#define SAVE_TMP_PATH "games.tmp"
#define SAVE_MAGIC    0x534D4147u  // "GAMS"

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t length;
    uint32_t crc32;
} SaveHeader;

typedef struct {
    SaveHeader header;
    SaveData data;
} SaveFile;

static uint32_t crc32_calc(const void* data, uint32_t len) {
    const uint8_t* p = data;
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

bool save_load(SaveData* out) {
    SaveFile file;
    int32_t n = hal_fs_read(SAVE_PATH, &file, sizeof(file));
    if (n < 0) return false;

    if (n < (int32_t)sizeof(SaveHeader) || file.header.magic != SAVE_MAGIC) {
        printf("[Save] 存档格式不对，忽略\n");
        return false;
    }
    if (file.header.version != SAVE_VERSION || file.header.length != sizeof(SaveData) ||
        n != (int32_t)sizeof(file)) {
        printf("[Save] 存档版本 %u 与当前版本 %u 不符，忽略\n",
               (unsigned)file.header.version, (unsigned)SAVE_VERSION);
        return false;
    }
    if (crc32_calc(&file.data, sizeof(SaveData)) != file.header.crc32) {
        printf("[Save] 存档校验失败，忽略\n");
        return false;
    }
    *out = file.data;
    return true;
}

bool save_write(const SaveData* data) {
    SaveFile file;
    memset(&file, 0, sizeof(file));
    file.header.magic = SAVE_MAGIC;
    file.header.version = SAVE_VERSION;
    file.header.length = sizeof(SaveData);
    file.data = *data;
    file.header.crc32 = crc32_calc(&file.data, sizeof(SaveData));

    if (!hal_fs_write(SAVE_TMP_PATH, &file, sizeof(file))) return false;
    return hal_fs_rename(SAVE_TMP_PATH, SAVE_PATH);
}
