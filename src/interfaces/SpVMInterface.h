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
    vector<double> denseVector;
    vector<double> result;
    vector<double> referenceResult;
    SpVMInterface(const string& n);
    explicit SpVMInterface( string  n, utils::CSRMatrix m, const vector<double> &v);
    void setMatrix(utils::CSRMatrix m);
    void setDenseVector(const vector<double> &v);
    virtual vector<double> computeMultiplication() const = 0;
    virtual vector<double> computeMultiplication(int numThreads) const = 0;
    virtual void runPreprocessing(void* arg) = 0;
    virtual void runMultiplications() = 0;
    virtual void runMultiplications(int numThreads) = 0;
    static vector<double> generateRandomDenseVector(size_t n);
    bool checkCorrectness() const;
    bool computeReferenceResult();
};


#endif //DELIVERABLE1_2025_2026_SPVMINTERFACE_H