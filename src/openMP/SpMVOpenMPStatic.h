//
// Created by universita on 30/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPMVOPENMP_H
#define DELIVERABLE1_2025_2026_SPMVOPENMP_H

#include "../interfaces/SpVMInterface.h"

class SpMVOpenMPStatic : public SpVMInterface{
public:
    SpMVOpenMPStatic() = default;
    SpMVOpenMPStatic(const string& n);
    explicit SpMVOpenMPStatic(const string& n, const utils::CSRMatrix &m, const vector<double> &v);
    void runPreprocessing(void* arg) override;
    vector<double> computeMultiplication(int numThreads) const override;
    vector<double> computeMultiplication() const override;
    void runMultiplications(int numThreads) override;
    void runMultiplications() override;
    ~SpMVOpenMPStatic() override;
};


#endif //DELIVERABLE1_2025_2026_SPMVOPENMP_H