//
// Created by universita on 30/10/25.
//

#include "SpMVOpenMPBinning.h"

#include <iostream>
#include <omp.h>
#include <stdexcept>
#include <cmath>

SpMVOpenMPBinning::SpMVOpenMPBinning(const string &n):SpVMInterface(n) {

}

SpMVOpenMPBinning::SpMVOpenMPBinning(const string &n, const utils::CSRMatrix &m, const vector<double> &v): SpVMInterface(n, m, v)  {

}

SpMVOpenMPBinning::~SpMVOpenMPBinning() {
}

vector<double> SpMVOpenMPBinning::computeMultiplication(int numThreads) const {
    const vector<double> &values = matrix.val;
    const vector<int> &rowPointer = matrix.rowPointer;
    const vector<int> &colIndex = matrix.colIndex;
    vector<double> result(rowPointer.size() - 1, 0.0);

    // se non ci sono bin definiti o binPointer non valido, fallback al parallelismo per riga
    if (binPointer.size() < 2) {
#pragma omp parallel for num_threads(numThreads)
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

    // parallelizzo sui bin
    int numBins = static_cast<int>(binPointer.size()) - 1;
#pragma omp parallel for num_threads(numThreads) schedule(dynamic)
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
    const int numThreads = *static_cast<int *>(arg);
    if (numThreads <= 0) throw std::invalid_argument("runPreprocessing: numThreads must be > 0");

    const float densityThreshold = 0.8f;
    int currBinNNZ = 0;

    const int nrows = static_cast<int>(matrix.rowPointer.size()) - 1;
    const int totalNNZ = static_cast<int>(matrix.val.size());

    int denom = numThreads;
    int nnzTarget = totalNNZ / denom;
    if (nnzTarget < 1) nnzTarget = 1;

    binPointer.clear();
    binPointer.push_back(0);

    for (int row = 0; row < nrows; ++row) {
        int row_nnz = matrix.rowPointer[row + 1] - matrix.rowPointer[row];

        // riga molto densa: chiudo il bin corrente (se presente) e creo un bin singolo
        if (matrix.cols > 0 && static_cast<float>(row_nnz) / matrix.cols >= densityThreshold) {
            if (currBinNNZ > 0) {
                binPointer.push_back(row);
                currBinNNZ = 0;
            }
            binPointer.push_back(row + 1);
            continue;
        }

        // se il bin corrente è vuoto e la riga da sola supera il target -> bin singolo
        if (currBinNNZ == 0 && row_nnz >= nnzTarget) {
            binPointer.push_back(row + 1);
            continue;
        }

        int candidate = currBinNNZ + row_nnz;

        // confronto delle distanze al target
        int distIfAdd = std::abs(candidate - nnzTarget);
        int distCurr  = std::abs(currBinNNZ - nnzTarget);

        // se aggiungere peggiora la distanza e il bin corrente non è vuoto -> chiudo bin
        if (currBinNNZ > 0 && distIfAdd > distCurr) {
            binPointer.push_back(row); // chiudo il bin prima della riga corrente
            // inizio nuovo bin con la riga corrente
            currBinNNZ = row_nnz;
            if (currBinNNZ >= nnzTarget) {
                binPointer.push_back(row + 1);
                currBinNNZ = 0;
            }
            continue;
        }

        // includo la riga nel bin corrente
        currBinNNZ = candidate;
        if (currBinNNZ >= nnzTarget) {
            binPointer.push_back(row + 1);
            currBinNNZ = 0;
        }
    }

    // chiudi l'ultimo bin se non già chiuso
    if (binPointer.empty() || binPointer.back() != nrows) {
        binPointer.push_back(nrows);
    }

    for (auto& elem: binPointer) {
        std::cout << elem << ", ";
    }
    std::cout << "total bins: " << (binPointer.size() > 0 ? binPointer.size() - 1 : 0) << "\n";
}