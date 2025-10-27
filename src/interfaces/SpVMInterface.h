//
// Created by universita on 13/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPVMINTERFACE_H
#define DELIVERABLE1_2025_2026_SPVMINTERFACE_H

#include <string>
#include <map>
#include <vector>
#include <random>

#include "../utils/MatrixReader.h"

using namespace std;

class SpVMInterface {
public:
    string modelName;
    virtual ~SpVMInterface() = default;
    utils::CSRMatrix matrix;
    SpVMInterface() = default;
    vector<float> denseVector;
    vector<float> result;
    SpVMInterface(const string& n);
    explicit SpVMInterface( string  n, utils::CSRMatrix m, const vector<float> &v);
    void setMatrix(utils::CSRMatrix m);
    void setDenseVector(const vector<float> &v);
    virtual vector<float> computeMultiplication() const = 0;
    virtual void runMultiplications() = 0;
    static vector<float> generateRandomDenseVector(size_t n);
    bool checkCorrectness() const;
};


#endif //DELIVERABLE1_2025_2026_SPVMINTERFACE_H