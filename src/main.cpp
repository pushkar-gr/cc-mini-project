#include "state.h"
#include "path.h"
#include "cow.h"
#include "whiteout.h"
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

static struct fuse_operations ops = {
    .getattr  = fs_getattr,
    .mkdir    = fs_mkdir,
    .unlink   = fs_unlink,
    .rmdir    = fs_rmdir,
    .chmod    = fs_chmod,
    .truncate = fs_truncate,
    .open     = fs_open,
    .read     = fs_read,
    .write    = fs_write,
    .release  = fs_release,
    .readdir  = fs_readdir,
    .create   = fs_create,
    .utimens  = fs_utimens,
};

static void usage(const char* prog) {
    fprintf(stderr, "Usage: %s <lowerdir> <upperdir> <mountpoint> [FUSE options]\n", prog);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    State* state = new State();

    // Resolve to absolute paths before FUSE daemonizes and changes CWD to "/".
    char lower_abs[PATH_MAX], upper_abs[PATH_MAX];
    if (!realpath(argv[1], lower_abs)) {
        fprintf(stderr, "Error: cannot resolve '%s': %s\n", argv[1], strerror(errno));
        delete state;
        return 1;
    }
    if (!realpath(argv[2], upper_abs)) {
        fprintf(stderr, "Error: cannot resolve '%s': %s\n", argv[2], strerror(errno));
        delete state;
        return 1;
    }
    state->lower_dir = lower_abs;
    state->upper_dir = upper_abs;

    // Validate that both directories exist.
    struct stat st;
    for (const auto& dir : {state->lower_dir, state->upper_dir}) {
        if (stat(dir.c_str(), &st) == -1 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Error: '%s' is not a valid directory\n", dir.c_str());
            delete state;
            return 1;
        }
    }

    // Build FUSE argv: strip lowerdir and upperdir, keep mountpoint + any FUSE flags.
    int fuse_argc = argc - 2;
    char** fuse_argv = new char*[fuse_argc];
    fuse_argv[0] = argv[0];
    for (int i = 1; i < fuse_argc; i++)
        fuse_argv[i] = argv[i + 2];

    int ret = fuse_main(fuse_argc, fuse_argv, &ops, state);
    delete[] fuse_argv;
    delete state;
    return ret;
}
