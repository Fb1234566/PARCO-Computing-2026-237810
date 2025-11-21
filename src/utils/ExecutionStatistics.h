//
// Created by universita on 13/10/25.
//

#ifndef DELIVERABLE1_2025_2026_EXECUTIONSTATISTICS_H
#define DELIVERABLE1_2025_2026_EXECUTIONSTATISTICS_H

#include "../interfaces/SpVMInterface.h"

namespace utils {
    class ExecutionStatistics {
    private:
        SpVMInterface& model;
        string matrix;
    public:
        ExecutionStatistics(SpVMInterface&  i, string matrix);
        void runSerial(int iteration, const string& reportPath) const;
        void runOpenMP(int iteration, const string& reportPath, int numThreads) const;

    };
}



#endif //DELIVERABLE1_2025_2026_EXECUTIONSTATISTICS_H