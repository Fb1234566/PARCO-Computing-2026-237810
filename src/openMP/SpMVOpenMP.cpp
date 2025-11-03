//
// Created by universita on 30/10/25.
//

#include "SpMVOpenMP.h"

#include <iostream>
#include <omp.h>
#include <stdexcept>

SpMVOpenMP::SpMVOpenMP(const string &n):SpVMInterface(n) {

}

SpMVOpenMP::SpMVOpenMP(const string &n, const utils::CSRMatrix &m, const vector<double> &v): SpVMInterface(n, m, v)  {

}

SpMVOpenMP::~SpMVOpenMP() {
}

vector<double> SpMVOpenMP::computeMultiplication(int numThreads) const {
    const vector<double> &values = matrix.val;
    const vector<int> &rowPointer = matrix.rowPointer;
    const vector<int> &colIndex = matrix.colIndex;
    vector<double> result(rowPointer.size() - 1, 0.0f);
#pragma omp parallel for num_threads(numThreads)
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

void SpMVOpenMP::runMultiplications() {
    throw logic_error("The model is running OpenMP, not serial");
}

vector<double> SpMVOpenMP::computeMultiplication() const {
    throw logic_error("The model is running OpenMP, not serial");
}


void SpMVOpenMP::runMultiplications(int numThreads) {
    result = computeMultiplication(numThreads);
}




