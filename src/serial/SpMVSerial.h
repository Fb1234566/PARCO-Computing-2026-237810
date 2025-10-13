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
        explicit SpMVSerial(const std::map<std::string, vector<float>> &m, const vector<float> &v);
        vector<float> computeMultiplication() const override;
        void runMultiplications(const bool& randomVector) override;
};


#endif //DELIVERABLE1_2025_2026_SPMVSERIAL_H