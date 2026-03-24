#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdio>

using namespace std;

string lower_dir = "./lower";
string upper_dir = "./upper";

int my_unlink(const char *path) {
    char upper_path[1024];
    char lower_path[1024];
    char whiteout_path[1024];

    // build paths
    snprintf(upper_path, sizeof(upper_path), "%s%s", upper_dir.c_str(), path);
    snprintf(lower_path, sizeof(lower_path), "%s%s", lower_dir.c_str(), path);

    // extract filename
    const char *filename = strrchr(path, '/');
    if (filename) filename++;
    else filename = path;

    // whiteout path
    snprintf(whiteout_path, sizeof(whiteout_path),
             "%s/.wh.%s", upper_dir.c_str(), filename);

    // CASE 1: file in upper_dir
    if (access(upper_path, F_OK) == 0) {
        cout << "Deleting from upper_dir\n";
        return unlink(upper_path);
    }

    // CASE 2: file in lower_dir
    if (access(lower_path, F_OK) == 0) {
        cout << "Creating whiteout file\n";
        FILE *fp = fopen(whiteout_path, "w");
        if (!fp) return -1;
        fclose(fp);
        return 0;
    }

    // CASE 3: not found
    cout << "File not found\n";
    return -1;
}

int main() {
    my_unlink("/test.txt");
    return 0;
}
