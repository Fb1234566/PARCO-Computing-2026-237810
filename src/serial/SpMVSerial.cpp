#include "SpMVSerial.h"
#include "../utils/PrintUtils.h" // aggiungi questa riga

#include <random>
#include <iostream>
#include <iomanip>
#include <chrono>

SpMVSerial::SpMVSerial(const string &n, const utils::CSRMatrix &m, const vector<double> &v) : SpVMInterface(n, m, v) {
}

vector<double> SpMVSerial::computeMultiplication() const {
    const vector<double> &values = matrix.val;
    const vector<int> &rowPointer = matrix.rowPointer;
    const vector<int> &colIndex = matrix.colIndex;
    vector<double> result(rowPointer.size() - 1, 0.0f);

    for (int row = 0; row < rowPointer.size() - 1; ++row) {
        double partial_sum = 0.0f;
        for (int idx = rowPointer[row]; idx < rowPointer[row + 1]; ++idx) {
            const int col = colIndex[idx];
            partial_sum += denseVector[col] * values[idx];
        }
        result[row] = partial_sum;
    }
    return result;
}

void SpMVSerial::runMultiplications() {
    result = computeMultiplication();
}

vector<double> SpMVSerial::computeMultiplication(int numThreads) const {
    throw logic_error("The model is running serial, not OpenMP");
}

void SpMVSerial::runMultiplications(int numThreads) {
    throw logic_error("The model is running serial, not OpenMP");
}

SpMVSerial::SpMVSerial(const string &n) : SpVMInterface(n) {
}

void SpMVSerial::runPreprocessing(void* arg) {
}
