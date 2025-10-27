
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "src/utils/MatrixReader.h"
#include "src/serial/SpMVSerial.h"
#include "src/utils/ExecutionStatistics.h"

int main() {
    namespace fs = std::filesystem;
    const fs::path dataset_dir = "datasets";
    SpMVSerial serial;

    try {
        if (!fs::exists(dataset_dir) || !fs::is_directory(dataset_dir)) {
            std::cerr << "Directory not found: " << dataset_dir << '\n';
            return 1;
        }

        utils::MatrixReader reader;
        for (auto const& entry : fs::recursive_directory_iterator(dataset_dir)) {
            if (!entry.is_regular_file()) continue;

            std::cout << "============================\n";
            auto fullPath = entry.path().string();
            std::cout << "File: " << fullPath << '\n';
            serial.modelName = "serial SpMV";
            serial.setMatrix(reader(fullPath.c_str()));
            std::vector<float> denseVector(6, 1.0f);
            serial.setDenseVector(denseVector);
            utils::ExecutionStatistics stats(serial);
            stats.run();
            serial.runMultiplications();
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return 2;
    }

    return 0;
}
