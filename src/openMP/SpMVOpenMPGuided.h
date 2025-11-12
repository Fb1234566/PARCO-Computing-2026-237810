//
// Created by universita on 30/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPMVOPENMP_H
#define DELIVERABLE1_2025_2026_SPMVOPENMP_H

#include "../interfaces/SpVMInterface.h"

class SpMVOpenMPGuided : public SpVMInterface{
public:
    SpMVOpenMPGuided() = default;
    SpMVOpenMPGuided(const string& n);
    explicit SpMVOpenMPGuided(const string& n, const utils::CSRMatrix &m, const vector<double> &v);
    void runPreprocessing(void* arg) override;
    vector<double> computeMultiplication(int numThreads) const override;
    vector<double> computeMultiplication() const override;
    void runMultiplications(int numThreads) override;
    void runMultiplications() override;
    ~SpMVOpenMPGuided() override;
};


#endif //DELIVERABLE1_2025_2026_SPMVOPENMP_H