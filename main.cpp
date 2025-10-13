// main.cpp (C++11)
#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

#include "src/utils/MatrixReader.h"
#include "src/serial/SpMVSerial.h"
#include "src/utils/ExecutionStatistics.h"

static bool is_directory(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_regular_file(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static void traverse_and_run(const std::string& root, SpMVSerial& serial) {
    DIR* dir = ::opendir(root.c_str());
    if (!dir) return;

    struct dirent* ent = NULL;
    while ((ent = ::readdir(dir)) != NULL) {
        if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0) continue;

        std::string fullPath = root;
        if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/') fullPath += '/';
        fullPath += ent->d_name;

        if (is_directory(fullPath)) {
            traverse_and_run(fullPath, serial);
        } else if (is_regular_file(fullPath)) {
            std::cout << "============================\n";
            utils::MatrixReader reader;
            std::cout << "File: " << fullPath << "\n";
            serial.setMatrix(reader(fullPath.c_str()));
            std::vector<float> denseVector(6, 1.0f);
            serial.setDenseVector(denseVector);
            utils::ExecutionStatistics stats(serial);
            stats.run();
            serial.runMultiplications(true);
        }
    }
    ::closedir(dir);
}

int main() {
    const std::string dataset_dir = "datasets";
    SpMVSerial serial;
    traverse_and_run(dataset_dir, serial);
    return 0;
}
