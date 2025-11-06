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

void utils::ExecutionStatistics::runOpenMP(int iteration, const string &reportPath, int numThreads) const {
    PrintUtils::printColored("Running " + model.modelName + "...", utils::PrintUtils::TerminalColor::CYAN);
    const auto n = new int(1);
    model.runPreprocessing(n);
    model.setDenseVector(SpVMInterface::generateRandomDenseVector(model.matrix.cols));
    PrintUtils::logToFile("Set dense vector");
    if (!model.computeReferenceResult()) {
        const string errorMsg = "Correctness check failed: could not compute reference result.";
        utils::PrintUtils::logToFile(errorMsg);
        utils::PrintUtils::printColored(errorMsg, utils::PrintUtils::TerminalColor::RED);
        throw out_of_range(errorMsg);
    }

    // warm-up
    model.runMultiplications(numThreads);
    utils::PrintUtils::logToFile("Run warm-up");
    const int iters = 15;
    PrintUtils::logToFile("Start testing with " + std::to_string(iters) + "iterations");
    DataTable d;
    d.AddColumn("Iteration");
    d.AddColumn("Execution_time", DATATYPES::Double);
    d.AddColumn("Num_Threads");
    std::string thread_msg = "Testing with " + std::to_string(numThreads) + " threads...";
    PrintUtils::logToFile(thread_msg);
    PrintUtils::printColored(thread_msg, PrintUtils::TerminalColor::YELLOW);
    const auto nThreads = new int(numThreads);
    model.runPreprocessing(nThreads);
    auto t0 = std::chrono::steady_clock::now();
    model.runMultiplications(numThreads);
    auto t1 = std::chrono::steady_clock::now();
    if (model.checkCorrectness()) {
        PrintUtils::printColored("Result is correct", PrintUtils::TerminalColor::GREEN);
        PrintUtils::logToFile(std::to_string(iteration) + "/" + std::to_string(iters) + " Result is correct");
    } else {
        PrintUtils::printColored("Result is incorrect", PrintUtils::TerminalColor::RED);
        PrintUtils::logToFile(std::to_string(iteration) + "/" + std::to_string(iters) + " Result is incorrect");
    }
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    d.AddRow(vector<Cell>{iteration, ms, numThreads});
    std::string msg = "Computation time: " + std::to_string(ms) + " ms";
    PrintUtils::logToFile(msg);
    utils::PrintUtils::printColored(msg, utils::PrintUtils::TerminalColor::GREEN);

    std::string safeName = model.modelName;
    std::replace(safeName.begin(), safeName.end(), ' ', '_');
    std::string safeMatrix = matrix;
    std::replace(safeMatrix.begin(), safeMatrix.end(), '/', '_');
    d.ExportToCSV(reportPath + std::string("stats_") + safeName + "_" + safeMatrix + ".csv");
}

void utils::ExecutionStatistics::runSerial(int iteration, const string &reportPath) const {
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
    DataTable d;
    d.AddColumn("Iteration");
    d.AddColumn("Execution_time", DATATYPES::Double);
    double t0 = omp_get_wtime();
    model.runMultiplications();
    double t1 = omp_get_wtime();
    if (model.checkCorrectness()) {
        PrintUtils::printColored("Result is correct", PrintUtils::TerminalColor::GREEN);
        PrintUtils::logToFile(std::to_string(iteration + 1) + " Result is correct");
    } else {
        PrintUtils::printColored("Result is incorrect", PrintUtils::TerminalColor::RED);
        PrintUtils::logToFile(std::to_string(iteration + 1) + " Result is incorrect");
    }
    double ms = (t1 - t0) * 1000.0;
    d.AddRow(vector<Cell>{iteration, ms});

    std::string msg = "Computation time: " + std::to_string(ms) + " ms";
    PrintUtils::logToFile(msg);
    utils::PrintUtils::printColored(msg, utils::PrintUtils::TerminalColor::GREEN);
    std::string safeName = model.modelName;
    std::replace(safeName.begin(), safeName.end(), ' ', '_');
    std::string safeMatrix = matrix;
    std::replace(safeMatrix.begin(), safeMatrix.end(), '/', '_');
    d.ExportToCSV(reportPath + std::string("stats_") + safeName + "_" + safeMatrix + ".csv");
}
