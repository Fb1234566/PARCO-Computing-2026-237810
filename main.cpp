#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#include <cstring>

#include "src/utils/MatrixReader.h"
#include "src/serial/SpMVSerial.h"
#include "src/utils/ExecutionStatistics.h"
#include "src/utils/PrintUtils.h"
#include "src/openMP/SpMVOpenMP.h"
#include "src/utils/DataTable.h"

static void traverse_directory(const std::string& dir, std::vector<std::string>& files) {
    DIR* dp = opendir(dir.c_str());
    if (!dp) {
        std::cerr << "Impossibile aprire directory: " << dir << " (" << std::strerror(errno) << ")\n";
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dp)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string full = dir + "/" + name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) {
            std::cerr << "stat fallito per: " << full << " (" << std::strerror(errno) << ")\n";
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            traverse_directory(full, files);
        } else if (S_ISREG(st.st_mode)) {
            files.push_back(full);
        }
    }
    closedir(dp);
}

int main() {
    const std::string dataset_dir = "datasets";
    SpMVSerial serial("serial SpMV");
    SpMVOpenMP openMP("openMP SpMV");
    utils::PrintUtils::logToFile("Program started");

    struct stat st;
    if (stat(dataset_dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "Directory not found: " << dataset_dir << '\n';
        return 1;
    }

    utils::MatrixReader reader;
    std::vector<std::string> files;
    traverse_directory(dataset_dir, files);

    for (const auto& fullPath : files) {
        std::cout << "============================\n";
        std::cout << "File: " << fullPath << '\n';
        utils::PrintUtils::logToFile("Started working on matrix " + fullPath);
        utils::CSRMatrix matrix = reader(fullPath);
        serial.setMatrix(matrix);
        utils::ExecutionStatistics statsSerial(serial, fullPath);
        statsSerial.run();
        openMP.setMatrix(matrix);
        utils::ExecutionStatistics statsOpenMp(openMP, fullPath);
        statsOpenMp.run();
        utils::PrintUtils::logToFile("Done working on matrix "+ fullPath);
    }

    return 0;
}
