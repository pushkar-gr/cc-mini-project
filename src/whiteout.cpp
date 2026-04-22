#include "whiteout.h"
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

bool is_whiteout(const std::string& upper_dir, const std::string& name) {
    std::string wh = upper_dir + "/" + WH_PREFIX + name;
    struct stat st;
    return lstat(wh.c_str(), &st) == 0;
}

int create_whiteout(const std::string& upper_dir, const std::string& name) {
    std::string wh = upper_dir + "/" + WH_PREFIX + name;
    int fd = open(wh.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}

int remove_whiteout(const std::string& upper_dir, const std::string& name) {
    std::string wh = upper_dir + "/" + WH_PREFIX + name;
    if (unlink(wh.c_str()) == -1 && errno != ENOENT) return -errno;
    return 0;
}

// split_upper splits a FUSE path into the upper-layer parent directory and the
// filename component.
static void split_upper(const char* fuse_path,
                        std::string& upper_par, std::string& name) {
    State* s = get_state();
    std::string rel(fuse_path + 1);
    size_t slash = rel.rfind('/');
    if (slash == std::string::npos) {
        upper_par = s->upper_dir;
        name = rel;
    } else {
        upper_par = s->upper_dir + "/" + rel.substr(0, slash);
        name = rel.substr(slash + 1);
    }
}

int fs_unlink(const char* path) {
    State* s = get_state();
    std::string rel(path + 1);

    std::string upper_par, name;
    split_upper(path, upper_par, name);

    std::string upper_path = upper_par + "/" + name;

    size_t slash = rel.rfind('/');
    std::string lower_par = s->lower_dir +
        (slash != std::string::npos ? "/" + rel.substr(0, slash) : "");
    std::string lower_path = lower_par + "/" + name;

    struct stat st;
    bool in_upper = (lstat(upper_path.c_str(), &st) == 0);
    bool in_lower = (lstat(lower_path.c_str(), &st) == 0);

    if (!in_upper && !in_lower) return -ENOENT;

    // Remove the upper copy if it exists.
    if (in_upper && unlink(upper_path.c_str()) == -1) return -errno;

    // If the file also existed in lower, create a whiteout to hide it.
    if (in_lower) {
        if (create_whiteout(upper_par, name) != 0) return -EIO;
    }

    return 0;
}

int fs_rmdir(const char* path) {
    State* s = get_state();
    std::string rel(path + 1);

    std::string upper_par, name;
    split_upper(path, upper_par, name);

    std::string upper_path = upper_par + "/" + name;

    size_t slash = rel.rfind('/');
    std::string lower_par = s->lower_dir +
        (slash != std::string::npos ? "/" + rel.substr(0, slash) : "");
    std::string lower_path = lower_par + "/" + name;

    struct stat st;
    bool in_upper = (lstat(upper_path.c_str(), &st) == 0);
    bool in_lower = (lstat(lower_path.c_str(), &st) == 0);

    if (!in_upper && !in_lower) return -ENOENT;

    if (in_upper) {
        if (rmdir(upper_path.c_str()) == -1) {
            if (errno == ENOTEMPTY) return -ENOTEMPTY;
            return -errno;
        }
    }

    if (in_lower) {
        if (create_whiteout(upper_par, name) != 0) return -EIO;
    }

    return 0;
}
