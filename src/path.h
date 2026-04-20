#pragma once
#include "state.h"
#include <string>
#include <sys/stat.h>

std::string resolve_path(const char* fuse_path);
int fs_getattr(const char* path, struct stat* st);
int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info* fi);
int fs_read(const char* path, char* buf, size_t size, off_t offset,
            struct fuse_file_info* fi);
int fs_release(const char* path, struct fuse_file_info* fi);
int fs_utimens(const char* path, const struct timespec ts[2]);
int fs_chmod(const char* path, mode_t mode);
