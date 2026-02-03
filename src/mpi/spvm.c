#include  "spvm.h"
#include  "io.h"

void computeSpvmSerial(CSRMatrix* in, Vector* vec, Vector* res){
		int row;
		int idx;
		for(row = 0; row < in->rows; row++) {
				double partialSum = 0.0f;
				for(idx = in->rowPtr[row]; idx < in->rowPtr[row + 1]; ++idx){
						const int col = in->col[idx];
						partialSum += vec->val[col] * in->val[idx];
				}
				res->val[row] = partialSum;
		}
}

long long computeFlops(CSRMatrix* in){
		// For each non-zero element: 1 multiplication + 1 addition = 2 FLOPs
		return (long long)(in->nnz) * 2;
}

