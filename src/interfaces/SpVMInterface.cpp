//
// Created by universita on 13/10/25.
//

#include "SpVMInterface.h"

#include <utility>
#include <cmath>
#include <algorithm>

#include "src/utils/PrintUtils.h"

SpVMInterface::SpVMInterface(string  n, utils::CSRMatrix m, const vector<double> &v): modelName(std::move(n)), matrix(std::move(m)), denseVector(v){
}

void SpVMInterface::setMatrix(utils::CSRMatrix m) {
    matrix = std::move(m);
}

void SpVMInterface::setDenseVector(const vector<double> &v) {
    denseVector = v;
}

vector<double> SpVMInterface::generateRandomDenseVector(size_t n) {
    vector<double> vec(n);
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.0f, 1.0f);
    for (size_t i = 0; i < n; ++i) {
        vec[i] = dist(gen);
    }
    return vec;
}

SpVMInterface::SpVMInterface(const string& n): modelName(n){
}

bool SpVMInterface::computeReferenceResult() {

    utils::PrintUtils::logToFile("Starting reference result computation.");
    referenceResult = vector<double>(matrix.rows, 0.0);

    for (size_t i = 0; i < matrix.rowPointer.size() - 1; ++i) {
        double sum = 0.0;
        for (int j = matrix.rowPointer[i]; j < matrix.rowPointer[i + 1]; ++j) {
            if (static_cast<size_t>(matrix.colIndex[j]) >= denseVector.size()) {
                string errorMsg = "Error in reference computation: column index " + to_string(matrix.colIndex[j]) +
                                  " is out of bounds for dense vector of size " + to_string(denseVector.size()) + ".";
                utils::PrintUtils::logToFile(errorMsg);
                return false;
            }
            sum += matrix.val[j] * denseVector[matrix.colIndex[j]];
        }
        referenceResult[i] = sum;
    }

    utils::PrintUtils::logToFile("Reference result computation completed successfully.");
    return true;
}

bool SpVMInterface::checkCorrectness() const {
    utils::PrintUtils::logToFile("Starting result correctness check.");
    if (static_cast<size_t>(matrix.rows) != result.size()) {
        string errorMsg = "Correctness check failed: result size (" + to_string(result.size()) +
                          ") does not match matrix rows (" + to_string(matrix.rows) + ").";
        utils::PrintUtils::logToFile(errorMsg);
        utils::PrintUtils::printColored(errorMsg, utils::PrintUtils::TerminalColor::RED);
        return false;
    }

    const double epsilon = 1e-5;
    for (int i = 0; i < matrix.rows; ++i) {
        if (std::abs(referenceResult[i] - result[i]) > epsilon) {
            string errorMsg = "Correctness check failed: mismatch found at index " + to_string(i) +
                              ". Expected: " + to_string(referenceResult[i]) + ", Got: " + to_string(result[i]);
            utils::PrintUtils::logToFile(errorMsg);
            utils::PrintUtils::printColored(errorMsg, utils::PrintUtils::TerminalColor::RED);
            return false;
        }
    }

    string successMsg = "Correctness check passed successfully.";
    utils::PrintUtils::logToFile(successMsg);
    utils::PrintUtils::printColored(successMsg, utils::PrintUtils::TerminalColor::GREEN);
    return true;
}