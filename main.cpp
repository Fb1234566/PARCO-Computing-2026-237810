#include <iostream>
#include <filesystem>

#include "src/utils/MatrixReader.h"
#include "src/serial/SpMVSerial.h"
#include "src/utils/ExecutionStatistics.h"

int main() {
    const std::filesystem::path dataset_dir = "datasets";
    SpMVSerial serial;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dataset_dir)) {
        if (entry.is_regular_file()) {
            cout << "============================" << endl;
            utils::MatrixReader reader;
            cout << "File: " << entry.path() << endl;
            serial.setMatrix(reader(entry.path()));
            vector<float> denseVector (6, 1.0f);
            serial.setDenseVector(denseVector);
            utils::ExecutionStatistics stats(serial);
            stats.run();
            serial.runMultiplications(true);
        }
    }

    return 0;
}