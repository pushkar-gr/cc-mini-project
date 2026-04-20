#include "path.h"
#include <cerrno>
#include <cstring>
#include <sys/stat.h>

// resolve_path maps a FUSE path (e.g. "/dir/file") to the real OS path.
// Resolution order:
//   1. Whiteout in upper layer  -> return "" (deleted)
//   2. File in upper layer      -> return upper path
//   3. File in lower layer      -> return lower path
//   4. Not found                -> return ""
std::string resolve_path(const char* fuse_path) {
    State* s = get_state();

    if (strcmp(fuse_path, "/") == 0)
        return s->upper_dir;

    std::string rel(fuse_path + 1);   // strip leading '/'
    size_t slash = rel.rfind('/');
    std::string name = (slash == std::string::npos) ? rel : rel.substr(slash + 1);
    std::string par  = (slash == std::string::npos) ? "" : rel.substr(0, slash);

    std::string upper_par = s->upper_dir + (par.empty() ? "" : "/" + par);
    std::string lower_par = s->lower_dir + (par.empty() ? "" : "/" + par);

    // 1. Check for whiteout marker in upper layer.
    std::string wh = upper_par + "/" + WH_PREFIX + name;
    struct stat st;
    if (lstat(wh.c_str(), &st) == 0)
        return "";   // entry is deleted

    // 2. Upper layer.
    std::string up = upper_par + "/" + name;
    if (lstat(up.c_str(), &st) == 0)
        return up;

    // 3. Lower layer.
    std::string lo = lower_par + "/" + name;
    if (lstat(lo.c_str(), &st) == 0)
        return lo;

    return "";
}
