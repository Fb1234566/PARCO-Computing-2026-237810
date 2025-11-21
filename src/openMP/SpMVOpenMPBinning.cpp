#include "SpMVOpenMPBinning.h"

#include <iostream>
#include <omp.h>
#include <stdexcept>
#include <cmath>

SpMVOpenMPBinning::SpMVOpenMPBinning(const string &n):SpVMInterface(n) {}

SpMVOpenMPBinning::SpMVOpenMPBinning(const string &n, const utils::CSRMatrix &m, const vector<double> &v): SpVMInterface(n, m, v)  {}

SpMVOpenMPBinning::~SpMVOpenMPBinning() {}

vector<double> SpMVOpenMPBinning::computeMultiplication(int numThreads) const {
    const vector<double> &values = matrix.val;
    const vector<int> &rowPointer = matrix.rowPointer;
    const vector<int> &colIndex = matrix.colIndex;
    vector<double> result(rowPointer.size() - 1, 0.0);

    // Fallback logic
    if (binPointer.size() < 2) {
#pragma omp parallel for num_threads(numThreads) schedule(guided)
        for (int row = 0; row < static_cast<int>(rowPointer.size() - 1); ++row) {
            double partial_sum = 0.0;
            for (int idx = rowPointer[row]; idx < rowPointer[row + 1]; ++idx) {
                const int col = colIndex[idx];
                partial_sum += denseVector[col] * values[idx];
            }
            result[row] = partial_sum;
        }
        return result;
    }

    int numBins = static_cast<int>(binPointer.size()) - 1;

#pragma omp parallel for num_threads(numThreads) schedule(guided)
    for (int b = 0; b < numBins; ++b) {
        for (int row = binPointer[b]; row < binPointer[b + 1]; ++row) {
            double partial_sum = 0.0;
            for (int idx = rowPointer[row]; idx < rowPointer[row + 1]; ++idx) {
                const int col = colIndex[idx];
                partial_sum += denseVector[col] * values[idx];
            }
            result[row] = partial_sum;
        }
    }

    return result;
}

void SpMVOpenMPBinning::runMultiplications() {
    throw logic_error("The model is running OpenMP, not serial");
}

vector<double> SpMVOpenMPBinning::computeMultiplication() const {
    throw logic_error("The model is running OpenMP, not serial");
}


void SpMVOpenMPBinning::runMultiplications(int numThreads) {
    result = computeMultiplication(numThreads);
}


void SpMVOpenMPBinning::runPreprocessing(void* arg) {
    //Binning
    if (!arg) throw std::invalid_argument("runPreprocessing: arg is null");

    const int nnzTarget = 5000;

    if (nnzTarget <= 0) throw std::invalid_argument("runPreprocessing: nnzTarget must be > 0");

    const float densityThreshold = 0.8f;
    int currBinNNZ = 0;

    const int nrows = static_cast<int>(matrix.rowPointer.size()) - 1;

    binPointer.clear();
    binPointer.push_back(0);

    for (int row = 0; row < nrows; ++row) {
        int row_nnz = matrix.rowPointer[row + 1] - matrix.rowPointer[row];

        // Dense row logic
        if (matrix.cols > 0 && static_cast<float>(row_nnz) / matrix.cols >= densityThreshold) {
            if (currBinNNZ > 0) {
                binPointer.push_back(row);
                currBinNNZ = 0;
            }
            binPointer.push_back(row + 1);
            continue;
        }

        // Single-row bin logic
        if (currBinNNZ == 0 && row_nnz >= nnzTarget) {
            binPointer.push_back(row + 1);
            continue;
        }

        int candidate = currBinNNZ + row_nnz;
        int distIfAdd = std::abs(candidate - nnzTarget);
        int distCurr  = std::abs(currBinNNZ - nnzTarget);

        // Greedy choice
        if (currBinNNZ > 0 && distIfAdd > distCurr) {
            binPointer.push_back(row);
            currBinNNZ = row_nnz;
            if (currBinNNZ >= nnzTarget) {
                binPointer.push_back(row + 1);
                currBinNNZ = 0;
            }
            continue;
        }

        currBinNNZ = candidate;
        if (currBinNNZ >= nnzTarget) {
            binPointer.push_back(row + 1);
            currBinNNZ = 0;
        }
    }

    // Close final bin
    if (binPointer.empty() || binPointer.back() != nrows) {
        binPointer.push_back(nrows);
    }
}