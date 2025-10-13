//
// Created by universita on 13/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPVMINTERFACE_H
#define DELIVERABLE1_2025_2026_SPVMINTERFACE_H

#include <string>
#include <map>
#include <vector>
#include <random>

using namespace std;

class SpVMInterface {
public:
    virtual ~SpVMInterface() = default;

    SpVMInterface() = default;
    map<string, vector<float>> matrix;
    vector<float> denseVector;
    vector<float> result;
    explicit SpVMInterface(const std::map<std::string, vector<float>> &m, const vector<float> &v);
    void setMatrix(const map<std::string, vector<float>> &m);
    void setDenseVector(const vector<float> &v);
    virtual vector<float> computeMultiplication() const = 0;
    virtual void runMultiplications(const bool& randomVector) = 0;
    static vector<float> generateRandomDenseVector(size_t n);
    bool checkCorrectness(const vector<float>& v);
};


#endif //DELIVERABLE1_2025_2026_SPVMINTERFACE_H