#include "SpMVSerial.h"
#include "../utils/PrintUtils.h" // aggiungi questa riga

#include <random>
#include <iostream>
#include <iomanip>

SpMVSerial::SpMVSerial(const map<std::string, vector<float> > &m, const vector<float> &v): SpVMInterface(m, v) {
}

vector<float> SpMVSerial::computeMultiplication() const {
    const vector<float> &values = matrix.at("vals");
    const vector<float> &rowPointer = matrix.at("rowPointer");
    const vector<float> &colIndex = matrix.at("elemIndex");
    vector<float> result(rowPointer.size() - 1, 0.0f);


    utils::PrintUtils::printColored("Starting serial SpVM...", utils::PrintUtils::TerminalColor::CYAN);

    for (int row = 0; row < rowPointer.size() - 1; ++row) {
        float partial_sum = 0.0f;
        for (int idx = static_cast<int>(rowPointer[row]); idx < static_cast<int>(rowPointer[row + 1]); ++idx) {
            const int col = static_cast<int>(colIndex[idx]);
            partial_sum += denseVector[col] * values[idx];
        }
        result[row] = partial_sum;
    }

    utils::PrintUtils::printColored("Multiplication complete!", utils::PrintUtils::TerminalColor::GREEN);

    return result;
}

void SpMVSerial::runMultiplications(const bool &randomVector) {
    const vector<float> &rowPointer = matrix.at("rowPointer");

    if (randomVector) {
        utils::PrintUtils::printColored("Generating dense vector...", utils::PrintUtils::TerminalColor::CYAN);
        denseVector = generateRandomDenseVector(rowPointer.size() - 1);
        utils::PrintUtils::printColored("Dense vector generated.", utils::PrintUtils::TerminalColor::GREEN);
    }

    result = computeMultiplication();
    utils::PrintUtils::printColored("Result saved as correct.", utils::PrintUtils::TerminalColor::GREEN);


}

