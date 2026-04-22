#pragma once

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <string>

static const char WH_PREFIX[]   = ".wh.";
static const size_t WH_PREFIX_LEN = 4;

// Global filesystem state holding the two layer paths.
struct State {
    std::string lower_dir;
    std::string upper_dir;
};

// Retrieve the private_data pointer set during fuse_main.
inline State* get_state() {
    return static_cast<State*>(fuse_get_context()->private_data);
}

inline int fh_to_fd(uint64_t fh) { return static_cast<int>(fh); }
