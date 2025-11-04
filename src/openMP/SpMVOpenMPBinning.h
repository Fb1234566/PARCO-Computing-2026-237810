//
// Created by universita on 30/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPMVOPENMPBINNING_H
#define DELIVERABLE1_2025_2026_SPMVOPENMPBINNING_H


#include "../interfaces/SpVMInterface.h"

class SpMVOpenMPBinning : public SpVMInterface{
private:
    vector<int> binPointer;
public:
    SpMVOpenMPBinning() = default;
    SpMVOpenMPBinning(const string& n);
    explicit SpMVOpenMPBinning(const string& n, const utils::CSRMatrix &m, const vector<double> &v);
    vector<double> computeMultiplication(int numThreads) const override;
    vector<double> computeMultiplication() const override;
    void runPreprocessing(void* arg) override;
    void runMultiplications(int numThreads) override;
    void runMultiplications() override;
    ~SpMVOpenMPBinning() override;
};


#endif //DELIVERABLE1_2025_2026_SPMVOPENMPBINNING_H