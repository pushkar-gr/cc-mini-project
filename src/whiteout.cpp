#include "whiteout.h"
#include <cerrno>
#include <dirent.h>
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

static int ensure_upper_par(const std::string& rel) {
    State* s = get_state();
    size_t slash = rel.rfind('/');
    if (slash == std::string::npos)
        return 0;

    std::string par_rel = rel.substr(0, slash);
    size_t pos = 0;
    while (pos <= par_rel.size()) {
        size_t next = par_rel.find('/', pos);
        std::string part = (next == std::string::npos)
                               ? par_rel
                               : par_rel.substr(0, next);
        if (!part.empty()) {
            std::string udir = s->upper_dir + "/" + part;
            struct stat st;
            if (stat(udir.c_str(), &st) == -1) {
                mode_t mode = 0755;
                std::string ldir = s->lower_dir + "/" + part;
                if (stat(ldir.c_str(), &st) == 0)
                    mode = st.st_mode;
                if (mkdir(udir.c_str(), mode) == -1 && errno != EEXIST)
                    return -errno;
            }
        }
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    return 0;
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
        DIR* dp = opendir(upper_path.c_str());
        if (dp) {
            struct dirent* de;
            while ((de = readdir(dp))) {
                std::string n = de->d_name;
                if (n == "." || n == "..") continue;
                if (n.size() > WH_PREFIX_LEN &&
                    n.compare(0, WH_PREFIX_LEN, WH_PREFIX) == 0)
                    continue; // whiteout marker, not a real entry
                closedir(dp);
                return -ENOTEMPTY;
            }
            closedir(dp);
        }
    }

    // Lower scan: any entry not covered by a whiteout in upper means non-empty.
    if (in_lower) {
        DIR* dp = opendir(lower_path.c_str());
        if (dp) {
            struct dirent* de;
            while ((de = readdir(dp))) {
                std::string n = de->d_name;
                if (n == "." || n == "..") continue;
                std::string wh = upper_path + "/" + WH_PREFIX + n;
                if (lstat(wh.c_str(), &st) == 0) continue; // covered by whiteout
                closedir(dp);
                return -ENOTEMPTY;
            }
            closedir(dp);
        }
    }

    if (in_upper) {
        DIR* dp = opendir(upper_path.c_str());
        if (dp) {
            struct dirent* de;
            while ((de = readdir(dp))) {
                std::string n = de->d_name;
                if (n == "." || n == "..") continue;
                std::string entry = upper_path + "/" + n;
                unlink(entry.c_str());
            }
            closedir(dp);
        }
        if (rmdir(upper_path.c_str()) == -1) return -errno;
    }

    if (in_lower) {
        int err = ensure_upper_par(rel);
        if (err != 0) return err;
        if (create_whiteout(upper_par, name) != 0) return -EIO;
    }

    return 0;
}
