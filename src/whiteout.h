#pragma once
#include "state.h"
#include <string>

bool is_whiteout(const std::string& upper_dir, const std::string& name);
int  create_whiteout(const std::string& upper_dir, const std::string& name);
int  remove_whiteout(const std::string& upper_dir, const std::string& name);
int fs_unlink(const char* path);
int fs_rmdir(const char* path);
