#include "path.h"
#include <cerrno>
#include <cstring>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>

std::string resolve_path(const char* fuse_path) {
    State* s = get_state();

    if (strcmp(fuse_path, "/") == 0)
        return s->upper_dir;

    std::string rel(fuse_path + 1);
    size_t slash = rel.rfind('/');
    std::string name = (slash == std::string::npos) ? rel : rel.substr(slash + 1);
    std::string par  = (slash == std::string::npos) ? "" : rel.substr(0, slash);

    std::string upper_par = s->upper_dir + (par.empty() ? "" : "/" + par);
    std::string lower_par = s->lower_dir + (par.empty() ? "" : "/" + par);

    std::string wh = upper_par + "/" + WH_PREFIX + name;
    struct stat st;
    if (lstat(wh.c_str(), &st) == 0)
        return "";

    std::string up = upper_par + "/" + name;
    if (lstat(up.c_str(), &st) == 0)
        return up;

    std::string lo = lower_par + "/" + name;
    if (lstat(lo.c_str(), &st) == 0)
        return lo;

    return "";
}

int fs_getattr(const char* path, struct stat* st) {
    if (strcmp(path, "/") == 0) {
        memset(st, 0, sizeof(*st));
        st->st_mode  = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }
    std::string real = resolve_path(path);
    if (real.empty()) return -ENOENT;
    if (lstat(real.c_str(), st) == -1) return -errno;
    return 0;
}

int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info* fi) {
    State* s = get_state();
    (void)offset; (void)fi;

    filler(buf, ".",  NULL, 0);
    filler(buf, "..", NULL, 0);

    std::string upper_dir, lower_dir;
    if (strcmp(path, "/") == 0) {
        upper_dir = s->upper_dir;
        lower_dir = s->lower_dir;
    } else {
        std::string rel(path + 1);
        upper_dir = s->upper_dir + "/" + rel;
        lower_dir = s->lower_dir + "/" + rel;
    }

    std::set<std::string> seen;

    // Enumerate upper layer — skip whiteout marker files.
    DIR* dp = opendir(upper_dir.c_str());
    if (dp) {
        struct dirent* de;
        while ((de = readdir(dp))) {
            std::string name = de->d_name;
            if (name == "." || name == "..") continue;
            if (name.size() > WH_PREFIX_LEN &&
                name.compare(0, WH_PREFIX_LEN, WH_PREFIX) == 0)
                continue;
            seen.insert(name);
            filler(buf, name.c_str(), NULL, 0);
        }
        closedir(dp);
    }

    // Enumerate lower layer — skip entries already seen or whited out.
    dp = opendir(lower_dir.c_str());
    if (dp) {
        struct dirent* de;
        while ((de = readdir(dp))) {
            std::string name = de->d_name;
            if (name == "." || name == "..") continue;
            if (seen.count(name)) continue;

            // Skip if a whiteout marker exists in upper.
            std::string wh = upper_dir + "/" + WH_PREFIX + name;
            struct stat st;
            if (lstat(wh.c_str(), &st) == 0) continue;

            seen.insert(name);
            filler(buf, name.c_str(), NULL, 0);
        }
        closedir(dp);
    }

    return 0;
}

int fs_read(const char* path, char* buf, size_t size, off_t offset,
            struct fuse_file_info* fi) {
    (void)path;
    int n = pread((int)fi->fh, buf, size, offset);
    return (n == -1) ? -errno : n;
}

int fs_release(const char* path, struct fuse_file_info* fi) {
    (void)path;
    if (fi->fh) close((int)fi->fh);
    return 0;
}

int fs_utimens(const char* path, const struct timespec ts[2]) {
    std::string real = resolve_path(path);
    if (real.empty()) return -ENOENT;

    // Only update timestamps on upper-layer files; lower is read-only.
    State* s = get_state();
    if (real.compare(0, s->upper_dir.size(), s->upper_dir) != 0)
        return 0;

    struct utimbuf times = { ts[0].tv_sec, ts[1].tv_sec };
    if (utime(real.c_str(), &times) == -1) return -errno;
    return 0;
}

int fs_chmod(const char* path, mode_t mode) {
    std::string real = resolve_path(path);
    if (real.empty()) return -ENOENT;

    State* s = get_state();
    if (real.compare(0, s->upper_dir.size(), s->upper_dir) != 0)
        return 0;

    if (chmod(real.c_str(), mode) == -1) return -errno;
    return 0;
}
