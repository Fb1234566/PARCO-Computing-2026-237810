//
// Created by universita on 13/10/25.
//

#include "SpVMInterface.h"
SpVMInterface::SpVMInterface(const map<std::string, vector<float> > &m, const vector<float> &v): matrix(m), denseVector(v) {
}

void SpVMInterface::setMatrix(const map<std::string, vector<float> > &m) {
    matrix = m;
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
