//
// Created by universita on 12/11/25.
//

#ifndef DELIVERABLE1_2025_2026_SPMVOPENMPDYNAMIC_H
#define DELIVERABLE1_2025_2026_SPMVOPENMPDYNAMIC_H

#include "../interfaces/SpVMInterface.h"

class SpMVOpenMPDynamic : public SpVMInterface{
public:
    SpMVOpenMPDynamic() = default;
    SpMVOpenMPDynamic(const string& n);
    explicit SpMVOpenMPDynamic(const string& n, const utils::CSRMatrix &m, const vector<double> &v);
    void runPreprocessing(void* arg) override;
    vector<double> computeMultiplication(int numThreads) const override;
    vector<double> computeMultiplication() const override;
    void runMultiplications(int numThreads) override;
    void runMultiplications() override;
    ~SpMVOpenMPDynamic() override;
};


#endif //DELIVERABLE1_2025_2026_SPMVOPENMPDYNAMIC_H