#ifndef DELIVERABLE2_2025_2026_DATA_FACTORY_H
#define DELIVERABLE2_2025_2026_DATA_FACTORY_H

#include "../mpi/io.h"

void createWeakScalingDataset(int rows, int cols, int nnz);

int writeCOOAsMatrixMarket(const char *filename, const COOMatrix *m);

#endif //DELIVERABLE2_2025_2026_DATA_FACTORY_H