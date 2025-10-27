//
// Created by universita on 13/10/25.
//

#include "SpVMInterface.h"

#include <utility>
#include <cmath>
#include <algorithm>
SpVMInterface::SpVMInterface(string  n, utils::CSRMatrix m, const vector<float> &v): modelName(std::move(n)), matrix(std::move(m)), denseVector(v){
}

void SpVMInterface::setMatrix(utils::CSRMatrix m) {
    matrix = std::move(m);
}

void SpVMInterface::setDenseVector(const vector<float> &v) {
    denseVector = v;
}

vector<float> SpVMInterface::generateRandomDenseVector(size_t n) {
    vector<float> vec(n);
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (size_t i = 0; i < n; ++i) {
        vec[i] = dist(gen);
    }
    return vec;
}

SpVMInterface::SpVMInterface(const string& n): modelName(n){
}

bool SpVMInterface::checkCorrectness() const {
    if (static_cast<size_t>(matrix.rows) != result.size()) {
        return false;
    }

    std::vector<float> referenceResult(matrix.rows, 0.0f);

    for (int i = 0; i < matrix.rows; ++i) {
        float sum = 0.0f;
        for (int j = matrix.rowPointer[i]; j < matrix.rowPointer[i + 1]; ++j) {
            if (static_cast<size_t>(matrix.colIndex[j]) >= denseVector.size()) {
                return false;
            }
            sum += matrix.val[j] * denseVector[matrix.colIndex[j]];
        }
        referenceResult[i] = sum;
    }

    const float epsilon = 1e-5f;
    for (int i = 0; i < matrix.rows; ++i) {
        if (std::abs(referenceResult[i] - result[i]) > epsilon) {
            return false;
        }
    }

    return true;
}
