//
// Created by universita on 09/10/25.
//

#ifndef DELIVERABLE1_2025_2026_SPMVSERIAL_H
#define DELIVERABLE1_2025_2026_SPMVSERIAL_H
#include <map>
#include <string>
#include <vector>
#include "../interfaces/SpVMInterface.h"
using namespace std;

class SpMVSerial: public SpVMInterface {
public:
        SpMVSerial() = default;
        SpMVSerial(const string& n);
        explicit SpMVSerial(const string& n, const utils::CSRMatrix &m, const vector<float> &v);
        vector<float> computeMultiplication() const override;
        void runMultiplications() override;
};


#endif //DELIVERABLE1_2025_2026_SPMVSERIAL_H