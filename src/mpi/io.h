//
// Created by universita on 11/12/25.
//

#ifndef DELIVERABLE1_2025_2026_IO_H
#define DELIVERABLE1_2025_2026_IO_H

#include <stdbool.h>

typedef struct COOEntry{
	int row, col;
	double val;
} COOEntry;

typedef struct vector {
    int len;
    double* val;
} Vector;

typedef struct COOMatrix{
    int rows;
    int cols;
    int nnz;
    int *row;
    int *col;
    double *val;
} COOMatrix;

typedef struct CSRMatrix{
    int rows;
    int cols;
    int nnz;
    int *rowPtr;
    int *col;
    double *val;
} CSRMatrix;

void readMatrixCOO(const char* path, COOMatrix* m);

void sortCOOMatrix(COOMatrix* m);

void COOToCSR(COOMatrix* in, CSRMatrix* out);

int COOEntryCompartor(const void* a, const void* b);

void randomInitCOO(COOMatrix* m, int rows, int cols, int nRanks, int nnz);

int generateRandInt(int min, int max);

double generateRandDouble();

void setRandSeed();

bool checkIfValueIsAlreadyPresent(COOEntry* c, COOEntry* e, int n);

void splitCOOMatrix(COOMatrix* inMatrix, COOMatrix* arrayMatrices, int P);

void initCOO(COOMatrix* m, int rows, int cols, int nnz);

void COOListToCSR(COOMatrix* in, CSRMatrix* out, int P);

void printCSR(const CSRMatrix* m);

void printCOO(const COOMatrix* m);

void printVector(const Vector* v);

void initVector(Vector* v, int len);

bool compareVectors(Vector* v1, Vector* v2);

#endif //DELIVERABLE1_2025_2026_IO_H
