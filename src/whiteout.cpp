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
