// src/openMP/main_openmp.cpp
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <filesystem>

#include "SpMVOpenMPStatic.h"
#include "../utils/MatrixReader.h"
#include "../utils/ExecutionStatistics.h"
#include "../utils/PrintUtils.h"

int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <matrix_path> <iterations> <output_dir> <nthreads>\n";
        return 1;
    }

    const std::string fullPath = argv[1];

    int iterations = 1;
    try {
        size_t pos = 0;
        long tmp = std::stol(argv[2], &pos);
        if (pos != std::strlen(argv[2]) || tmp <= 0) {
            std::cerr << "Errore: iterations deve essere un intero positivo\n";
            return 5;
        }
        iterations = static_cast<int>(tmp);
    } catch (const std::exception &e) {
        std::cerr << "Errore durante il parse di iterations: " << e.what() << '\n';
        return 5;
    }

    const std::string outputDirStr = argv[3];
    try {
        namespace fs = std::filesystem;
        fs::path outPath(outputDirStr);
        if (!fs::exists(outPath) || !fs::is_directory(outPath)) {
            std::cerr << "Errore: directory di output " << outputDirStr << " non esiste o non è una directory\n";
            return 6;
        }
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Errore filesystem: " << e.what() << '\n';
        return 6;
    }

    int nthreads = 1;
    try {
        size_t pos = 0;
        long tmp = std::stol(argv[4], &pos);
        if (pos != std::strlen(argv[4]) || tmp <= 0) {
            std::cerr << "Errore: nthreads deve essere un intero positivo\n";
            return 7;
        }
        nthreads = static_cast<int>(tmp);
    } catch (const std::exception &e) {
        std::cerr << "Errore durante il parse di nthreads: " << e.what() << '\n';
        return 7;
    }

    struct stat sb;
    if (stat(fullPath.c_str(), &sb) != 0) {
        std::cerr << "Errore: impossibile accedere a " << fullPath << ": " << std::strerror(errno) << '\n';
        return 2;
    }
    if (!S_ISREG(sb.st_mode)) {
        std::cerr << "Errore: " << fullPath << " non è un file regolare\n";
        return 3;
    }

    utils::PrintUtils::logToFile("Serial program started");
    utils::PrintUtils::logToFile(std::string("Output directory: ") + outputDirStr);
    try {
        SpMVOpenMPStatic openMP("OpenMP SpMV");
        utils::MatrixReader reader;

        std::cout << "============================\n";
        std::cout << "File: " << fullPath << '\n';
        std::cout << "Iterations: " << iterations << '\n';
        std::cout << "Output dir: " << outputDirStr << '\n';
        std::cout << "Nthreads: " << nthreads << '\n';
        utils::PrintUtils::logToFile("Started working on matrix " + fullPath);
        utils::PrintUtils::logToFile(std::string("Iterations: ") + std::to_string(iterations));
        utils::PrintUtils::logToFile(std::string("Nthreads: ") + std::to_string(nthreads));

        utils::CSRMatrix matrix = reader(fullPath);
        openMP.setMatrix(matrix);

        utils::ExecutionStatistics serialStats(openMP, fullPath);

        serialStats.runOpenMP(iterations, outputDirStr, nthreads);

        utils::PrintUtils::logToFile("Done working on matrix " + fullPath);
    } catch (const std::exception &e) {
        std::cerr << "Errore durante il caricamento o l'impostazione della matrice: " << e.what() << '\n';
        utils::PrintUtils::logToFile(std::string("Error: ") + e.what());
        return 4;
    }

    return 0;
}
