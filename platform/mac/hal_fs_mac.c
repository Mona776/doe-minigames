/**
 * 数据分区 - Mac 模拟器实现：映射到 sim_storage/data/ 文件夹
 */

#include <stdio.h>
#include <sys/stat.h>
#include "hal/hal_fs.h"

#ifndef PET_SIM_STORAGE_DIR
#error "PET_SIM_STORAGE_DIR 需要由 CMake 定义"
#endif

#define DATA_DIR PET_SIM_STORAGE_DIR "/data/"

static void full_path(const char* path, char* out, size_t size) {
    snprintf(out, size, DATA_DIR "%s", path);
}

bool hal_fs_init(void) {
    mkdir(PET_SIM_STORAGE_DIR, 0755);
    mkdir(DATA_DIR, 0755);
    struct stat st;
    return stat(DATA_DIR, &st) == 0 && S_ISDIR(st.st_mode);
}

int32_t hal_fs_read(const char* path, void* buf, uint32_t size) {
    char p[512];
    full_path(path, p, sizeof(p));
    FILE* f = fopen(p, "rb");
    if (f == NULL) return -1;
    size_t n = fread(buf, 1, size, f);
    fclose(f);
    return (int32_t)n;
}

bool hal_fs_write(const char* path, const void* data, uint32_t size) {
    char p[512];
    full_path(path, p, sizeof(p));
    FILE* f = fopen(p, "wb");
    if (f == NULL) return false;
    bool ok = fwrite(data, 1, size, f) == size;
    ok = (fflush(f) == 0) && ok;
    fclose(f);
    return ok;
}

bool hal_fs_rename(const char* from, const char* to) {
    char a[512], b[512];
    full_path(from, a, sizeof(a));
    full_path(to, b, sizeof(b));
    return rename(a, b) == 0;
}

bool hal_fs_remove(const char* path) {
    char p[512];
    full_path(path, p, sizeof(p));
    return remove(p) == 0;
}
