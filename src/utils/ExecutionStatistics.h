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
    public:
        ExecutionStatistics(SpVMInterface&  i);
        void run() const;

    };
}



#endif //DELIVERABLE1_2025_2026_EXECUTIONSTATISTICS_H