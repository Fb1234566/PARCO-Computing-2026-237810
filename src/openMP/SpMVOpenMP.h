//
// Created by universita on 30/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPMVOPENMP_H
#define DELIVERABLE1_2025_2026_SPMVOPENMP_H

#include "../interfaces/SpVMInterface.h"

class SpMVOpenMP : public SpVMInterface{
public:
    SpMVOpenMP() = default;
    SpMVOpenMP(const string& n);
    explicit SpMVOpenMP(const string& n, const utils::CSRMatrix &m, const vector<double> &v);
    void runPreprocessing(void* arg) override;
    vector<double> computeMultiplication(int numThreads) const override;
    vector<double> computeMultiplication() const override;
    void runMultiplications(int numThreads) override;
    void runMultiplications() override;
    ~SpMVOpenMP() override;
};


#endif //DELIVERABLE1_2025_2026_SPMVOPENMP_H