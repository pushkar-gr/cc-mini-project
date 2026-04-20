#pragma once
#include "state.h"
#include <string>

int copy_to_upper(const std::string& src, const std::string& dst);
int fs_open(const char* path, struct fuse_file_info* fi);
int fs_write(const char* path, const char* buf, size_t size, off_t offset,
             struct fuse_file_info* fi);
int fs_create(const char* path, mode_t mode, struct fuse_file_info* fi);
int fs_truncate(const char* path, off_t size);
int fs_mkdir(const char* path, mode_t mode);
