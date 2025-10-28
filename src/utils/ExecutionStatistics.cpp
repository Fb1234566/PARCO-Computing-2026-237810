//
// Created by universita on 13/10/25.
//

#include "ExecutionStatistics.h"


#include <iostream>
#include <chrono>
#include <string>
#include "PrintUtils.h"

#include <utility>

utils::ExecutionStatistics::ExecutionStatistics(SpVMInterface &i) : model(i) {
}

void utils::ExecutionStatistics::run() const {
    utils::PrintUtils::printColored("Running " + model.modelName + "...", utils::PrintUtils::TerminalColor::CYAN);

    model.setDenseVector(SpVMInterface::generateRandomDenseVector(model.matrix.rowPointer.size()));
    utils::PrintUtils::logToFile("Set dense vector");

    // warm-up
    model.runMultiplications();
    utils::PrintUtils::logToFile("Run warm-up");
    const int iters = 10;
    double best_ms = 1e9;
    double sum_ms = 0;
    PrintUtils::logToFile("Start testing with " + std::to_string(iters) + "iterations");
    for (int t = 0; t < iters; ++t) {
        auto t0 = std::chrono::steady_clock::now();
        model.runMultiplications();
        auto t1 = std::chrono::steady_clock::now();
        if (model.checkCorrectness()) {
            PrintUtils::printColored("Result is correct", PrintUtils::TerminalColor::GREEN);
            PrintUtils::logToFile(std::to_string(t+1)+ "/"+ std::to_string(iters) + " Result is correct");
        } else {
            PrintUtils::printColored("Result is incorrect", PrintUtils::TerminalColor::RED);
            PrintUtils::logToFile(std::to_string(t+1)+ "/"+ std::to_string(iters) + " Result is incorrect");
        }
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
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
}
