//
// Created by universita on 13/10/25.
//

#include "ExecutionStatistics.h"


#include <chrono>
#include <string>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <omp.h>
#include "PrintUtils.h"
#include "DataTable.h"

utils::ExecutionStatistics::ExecutionStatistics(SpVMInterface &i, string matrix) : model(i), matrix(std::move(matrix)) {
}

void utils::ExecutionStatistics::runOpenMP() const {
    PrintUtils::printColored("Running " + model.modelName + "...", utils::PrintUtils::TerminalColor::CYAN);

    model.setDenseVector(SpVMInterface::generateRandomDenseVector(model.matrix.cols));
    PrintUtils::logToFile("Set dense vector");
    if (!model.computeReferenceResult()) {
        const string errorMsg = "Correctness check failed: could not compute reference result.";
        utils::PrintUtils::logToFile(errorMsg);
        utils::PrintUtils::printColored(errorMsg, utils::PrintUtils::TerminalColor::RED);
        throw out_of_range(errorMsg);
    }

    // warm-up
    model.runMultiplications(1);
    utils::PrintUtils::logToFile("Run warm-up");
    const int iters = 15;
    PrintUtils::logToFile("Start testing with " + std::to_string(iters) + "iterations");
    DataTable d;
    d.AddColumn("Iteration");
    d.AddColumn("Execution_time", DATATYPES::Double);
    d.AddColumn("Num_Threads");
    const std::vector<int> threads_to_test = {1, 2, 4, 8, 12, 16, 20, 24, 32, 48, 64, 72, 96};
    for (const auto& tn: threads_to_test) {
        std::string thread_msg = "Testing with " + std::to_string(tn) + " threads...";
        PrintUtils::logToFile(thread_msg);
        PrintUtils::printColored(thread_msg, PrintUtils::TerminalColor::YELLOW);
        double best_ms = 1e9;
        double sum_ms = 0;
        for (int t = 0; t < iters; ++t) {
            auto t0 = std::chrono::steady_clock::now();
            model.runMultiplications(tn);
            auto t1 = std::chrono::steady_clock::now();
            if (model.checkCorrectness()) {
                PrintUtils::printColored("Result is correct", PrintUtils::TerminalColor::GREEN);
                PrintUtils::logToFile(std::to_string(t + 1) + "/" + std::to_string(iters) + " Result is correct");
            } else {
                PrintUtils::printColored("Result is incorrect", PrintUtils::TerminalColor::RED);
                PrintUtils::logToFile(std::to_string(t + 1) + "/" + std::to_string(iters) + " Result is incorrect");
            }
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            d.AddRow(vector<Cell>{t, ms, tn});
            if (ms < best_ms) best_ms = ms;
            sum_ms += ms;
        }
        std::string msg = "Best computation time: " + std::to_string(best_ms) + " ms";
        std::string avg = "Average computation time: " + std::to_string(sum_ms / iters) + " ms";
        PrintUtils::logToFile(msg);
        PrintUtils::logToFile(avg);
        utils::PrintUtils::printColored(msg, utils::PrintUtils::TerminalColor::GREEN);
        utils::PrintUtils::printColored(avg, utils::PrintUtils::TerminalColor::GREEN);
    }

    std::string safeName = model.modelName;
    std::replace(safeName.begin(), safeName.end(), ' ', '_');
    std::string safeMatrix = matrix;
    std::replace(safeMatrix.begin(), safeMatrix.end(), '/', '_');
    d.ExportToCSV("results/"+ std::string("stats_") + safeName + "_" + safeMatrix + ".csv");
}

void utils::ExecutionStatistics::runSerial() const {
    PrintUtils::printColored("Running " + model.modelName + "...", utils::PrintUtils::TerminalColor::CYAN);

    model.setDenseVector(SpVMInterface::generateRandomDenseVector(model.matrix.cols));
    PrintUtils::logToFile("Set dense vector");
    if (!model.computeReferenceResult()) {
        const string errorMsg = "Correctness check failed: could not compute reference result.";
        utils::PrintUtils::logToFile(errorMsg);
        utils::PrintUtils::printColored(errorMsg, utils::PrintUtils::TerminalColor::RED);
        throw out_of_range(errorMsg);
    }

    // warm-up
    model.runMultiplications();
    utils::PrintUtils::logToFile("Run warm-up");
    const int iters = 15;
    double best_ms = 1e9;
    double sum_ms = 0;
    PrintUtils::logToFile("Start testing with " + std::to_string(iters) + "iterations");
    DataTable d;
    d.AddColumn("Iteration");
    d.AddColumn("Execution_time", DATATYPES::Double);
    for (int t = 0; t < iters; ++t) {
        double t0 = omp_get_wtime();
        model.runMultiplications();
        double t1 = omp_get_wtime();
        if (model.checkCorrectness()) {
            PrintUtils::printColored("Result is correct", PrintUtils::TerminalColor::GREEN);
            PrintUtils::logToFile(std::to_string(t + 1) + "/" + std::to_string(iters) + " Result is correct");
        } else {
            PrintUtils::printColored("Result is incorrect", PrintUtils::TerminalColor::RED);
            PrintUtils::logToFile(std::to_string(t + 1) + "/" + std::to_string(iters) + " Result is incorrect");
        }
        double ms = (t1 - t0) * 1000.0;
        d.AddRow(vector<Cell>{t, ms});
        if (ms < best_ms) best_ms = ms;
        sum_ms += ms;
    }

    // stampa risultato
    std::string msg = "Best computation time: " + std::to_string(best_ms) + " ms";
    std::string avg = "Average computation time: " + std::to_string(sum_ms / iters) + " ms";
    PrintUtils::logToFile(msg);
    PrintUtils::logToFile(avg);
    utils::PrintUtils::printColored(msg, utils::PrintUtils::TerminalColor::GREEN);
    utils::PrintUtils::printColored(avg, utils::PrintUtils::TerminalColor::GREEN);
    std::string safeName = model.modelName;
    std::replace(safeName.begin(), safeName.end(), ' ', '_');
    std::string safeMatrix = matrix;
    std::replace(safeMatrix.begin(), safeMatrix.end(), '/', '_');
    d.ExportToCSV("results/"+ std::string("stats_") + safeName + "_" + safeMatrix + ".csv");
}

