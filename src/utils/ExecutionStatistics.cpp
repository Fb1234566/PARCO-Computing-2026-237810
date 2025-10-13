//
// Created by universita on 13/10/25.
//

#include "ExecutionStatistics.h"

#include <utility>

utils::ExecutionStatistics::ExecutionStatistics(SpVMInterface& i):model(i){
}

void utils::ExecutionStatistics::run() const {
    model.runMultiplications(true);
}

