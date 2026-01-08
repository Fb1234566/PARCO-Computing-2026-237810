//
// Created by universita on 11/12/25.
//

#ifndef DELIVERABLE1_2025_2026_IO_H
#define DELIVERABLE1_2025_2026_IO_H

typedef struct COOEntry{
	int row, col;
	double val;
} COOEntry;

typedef struct COOMatrix{
    int rows;
    int cols;
    int nnz;
    int *row;
    int *col;
    float *val;
} COOMatrix;

typedef struct CSRMatrix{
    int rows;
    int cols;
    int nnz;
    int *rowPtr;
    int *col;
    float *val;
} CSRMatrix;

void readMatrixCOO(const char* path, COOMatrix* m);

void sortCOOMatrix(COOMatrix* m);

void COOToCSR(COOMatrix* in, CSRMatrix* out);

int COOEntryCompartor(const void* a, const void* b);


#endif //DELIVERABLE1_2025_2026_IO_H