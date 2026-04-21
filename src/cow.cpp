#include "cow.h"
#include "path.h"
#include "whiteout.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// copy_to_upper copies a file from src (lower layer) to dst (upper layer),
// preserving its permissions. Used to implement Copy-on-Write.
int copy_to_upper(const std::string& src, const std::string& dst) {
    int src_fd = open(src.c_str(), O_RDONLY);
    if (src_fd == -1) return -errno;

    struct stat st;
    if (fstat(src_fd, &st) == -1) { close(src_fd); return -errno; }

    int dst_fd = open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (dst_fd == -1) { close(src_fd); return -errno; }

    char buf[4096];
    ssize_t n;
    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dst_fd, buf, (size_t)n) != n) {
            close(src_fd); close(dst_fd);
            return -EIO;
        }
    }

    close(src_fd);
    close(dst_fd);
    return (n == -1) ? -errno : 0;
}

int fs_open(const char* path, struct fuse_file_info* fi) {
    State* s = get_state();

    std::string real = resolve_path(path);
    if (real.empty()) return -ENOENT;

    std::string rel(path + 1);
    std::string upper_path = s->upper_dir + "/" + rel;

    // Copy-on-Write: if the file is opened for writing and lives in the lower
    // layer, copy it to the upper layer before opening.
    bool write_access = (fi->flags & (O_WRONLY | O_RDWR | O_APPEND)) != 0;
    if (write_access && real != upper_path) {
        int err = copy_to_upper(real, upper_path);
        if (err != 0) return err;
        real = upper_path;
    }

    int fd = open(real.c_str(), fi->flags);
    if (fd == -1) return -errno;

    fi->fh = (uint64_t)fd;
    return 0;
}

int fs_write(const char* path, const char* buf, size_t size, off_t offset,
             struct fuse_file_info* fi) {
    (void)path;
    int n = pwrite((int)fi->fh, buf, size, offset);
    return (n == -1) ? -errno : n;
}

int fs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
    State* s = get_state();
    std::string rel(path + 1);
    std::string upper_path = s->upper_dir + "/" + rel;

    // Remove any stale whiteout so the new file becomes visible.
    size_t slash = rel.rfind('/');
    std::string udir = s->upper_dir + (slash != std::string::npos ? "/" + rel.substr(0, slash) : "");
    std::string name = (slash != std::string::npos) ? rel.substr(slash + 1) : rel;
    remove_whiteout(udir, name);

    int fd = open(upper_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, mode);
    if (fd == -1) return -errno;

    fi->fh = (uint64_t)fd;
    return 0;
}

int fs_truncate(const char* path, off_t size) {
    State* s = get_state();
    std::string real = resolve_path(path);
    if (real.empty()) return -ENOENT;

    std::string rel(path + 1);
    std::string upper_path = s->upper_dir + "/" + rel;

    // Copy to upper before truncating if the file is in lower.
    if (real != upper_path) {
        int err = copy_to_upper(real, upper_path);
        if (err != 0) return err;
        real = upper_path;
    }

    if (truncate(real.c_str(), size) == -1) return -errno;
    return 0;
}

int fs_mkdir(const char* path, mode_t mode) {
    State* s = get_state();
    std::string rel(path + 1);
    std::string upper_path = s->upper_dir + "/" + rel;

    size_t slash = rel.rfind('/');
    std::string udir = s->upper_dir + (slash != std::string::npos ? "/" + rel.substr(0, slash) : "");
    std::string name = (slash != std::string::npos) ? rel.substr(slash + 1) : rel;
    remove_whiteout(udir, name);

    if (mkdir(upper_path.c_str(), mode) == -1) return -errno;
    return 0;
}
