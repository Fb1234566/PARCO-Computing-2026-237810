//
// Created by universita on 13/10/25.
//

#include "ExecutionStatistics.h"


#include <iostream>
#include <chrono>
#include <string>
#include "PrintUtils.h"

#include <utility>

utils::ExecutionStatistics::ExecutionStatistics(SpVMInterface& i):model(i){
}

void utils::ExecutionStatistics::run() const {
    utils::PrintUtils::printColored("Running " + model.modelName + "...", utils::PrintUtils::TerminalColor::CYAN);

    // genera e imposta il vettore denso
    model.setDenseVector(SpVMInterface::generateRandomDenseVector(model.matrix.rowPointer.size()));

    // warm-up
    model.runMultiplications();

    // numero di iterazioni per il benchmark (best of N)
    const int iters = 10;
    double best_ms = 1e9;
    double sum_ms = 0;

    for (int t = 0; t < iters; ++t) {
        auto t0 = std::chrono::steady_clock::now();
        model.runMultiplications();
        auto t1 = std::chrono::steady_clock::now();

        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms < best_ms) best_ms = ms;
        sum_ms+=ms;
    }

    // stampa risultato
    std::string msg = "Best computation time: " + std::to_string(best_ms) + " ms";
    std::string avg = "Average computation time: " + std::to_string(sum_ms/iters) + " ms";

    utils::PrintUtils::printColored(msg, utils::PrintUtils::TerminalColor::GREEN);
    utils::PrintUtils::printColored(avg, utils::PrintUtils::TerminalColor::GREEN);

}

