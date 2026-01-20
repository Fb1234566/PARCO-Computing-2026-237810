#include "spvm_mpi.h"

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include "io.h"
#include "spvm_mpi.h"
#include "logger.h"

typedef struct CSRheader{
	int rows, cols, nnz;
} CSRHeader;

int create_csr_header_type(MPI_Datatype *out_type) {
    CSRHeader tmp;
	MPI_Aint base, addr_rows, addr_cols, addr_nnz;
	MPI_Get_address(&tmp, &base);
	MPI_Get_address(&tmp.rows, &addr_rows);
	MPI_Get_address(&tmp.cols, &addr_cols);
	MPI_Get_address(&tmp.nnz,  &addr_nnz);

	MPI_Aint displs[3];
	displs[0] = addr_rows - base;
	displs[1] = addr_cols - base;
    displs[2] = addr_nnz  - base;
    int blocklens[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Type_create_struct(3, blocklens, displs, types, out_type);
    MPI_Type_commit(out_type);
	return 0;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);                     // Initialize MPI

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);// number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);// this process' rank
    
	// Create the data type to be sent via csr 
    MPI_Datatype csr_header_type;
	create_csr_header_type(&csr_header_type);
    
	CSRHeader* headers = NULL;
    CSRMatrix* procMatrices = NULL;
	int* bufPtr;
	int* bufCol;
	double* bufVal;
	int sendCountsOther[world_size];
	int sendCountsPtr[world_size];
	int dispPtr[world_size];
	int dispOther[world_size];


	// Start logger
    LOG_INFO("=== Program start ===");
        if (logger_init("app.log", LOG_LEVEL_INFO) != 0) {
	    fprintf(stderr, "Unable to initialize logger\n");
	    return 1;
    }

    printf("Hello from rank %d of %d\n", world_rank, world_size);
    if (world_rank == 0){
	    COOMatrix c1;
	    procMatrices = malloc(sizeof(CSRMatrix)*world_size);
        headers = malloc(sizeof(CSRHeader)*world_size);
		COOMatrix* cooMatrices = malloc(sizeof(COOMatrix)*world_size);
		setRandSeed();
		// Read the matrix as Coo
		randomInitCOO(&c1, 10, 10, world_size, world_size*10);
	    //readMatrixCOO("datasets/inline_1.mtx", &c1);
		bufCol = calloc(c1.nnz, sizeof(int));    
		bufPtr = calloc(c1.rows+1, sizeof(int));
	    bufVal = calloc(c1.nnz, sizeof(double));
	    splitCOOMatrix(&c1, cooMatrices, world_size);

		// Fill the headers in order to allow allocation of arrays
	    int i;
	    for(i=0; i<world_size; i++){
            headers[i].rows = cooMatrices[i].rows;
            headers[i].cols = cooMatrices[i].cols;
            headers[i].nnz = cooMatrices[i].nnz;
        }

		// Split the matrix into parts and convert them to CSR
		COOListToCSR(cooMatrices, procMatrices, world_size);
		free(cooMatrices);
		int offsetOther = 0;
		int offsetPtr = 0;
		//  Prepare the buffers, send counts and offsets in order to send the data vai scatterv
		for(i=0; i<world_size; i++){
			sendCountsOther[i] = procMatrices[i].nnz;
			dispOther[i] = offsetOther;
			offsetOther += sendCountsOther[i];

			sendCountsPtr[i] = procMatrices[i].rows+1;
			dispPtr[i] = offsetPtr;
			offsetPtr += sendCountsPtr[i];
			
			// Column index
			if (sendCountsOther[i] > 0 && procMatrices[i].col != NULL) {
				memcpy(bufCol + dispOther[i],                 /* dest (int*) */
				procMatrices[i].col,                     /* src (int*) */
				(size_t)sendCountsOther[i] * sizeof(bufCol[0])); /* bytes */
			}

			// Values
			if (sendCountsOther[i] > 0 && procMatrices[i].val != NULL) {
			     memcpy(bufVal + dispOther[i],                 /* dest (int*) */
			     procMatrices[i].val,                     /* src (int*) */
			     (size_t)sendCountsOther[i] * sizeof(bufVal[0])); /* bytes */
			}
					// Values
			if (sendCountsPtr[i] > 0 && procMatrices[i].rowPtr != NULL) {
			     memcpy(bufPtr + dispPtr[i],                 /* dest (int*) */
			     procMatrices[i].rowPtr,                     /* src (int*) */
			     (size_t)sendCountsPtr[i] * sizeof(bufPtr[0])); /* bytes */
			}
		}
    }
	
	// send the previously compiled headers
    CSRHeader myhdr;
	int* csrPtr, csrCol;
	double* csrVal;
	MPI_Scatter(headers, 1, csr_header_type,
	&myhdr,  1, csr_header_type,
	0, MPI_COMM_WORLD);
	
	LOG_INFO("rank %d got header: rows=%d cols=%d nnz=%d\n",
	world_rank, myhdr.rows, myhdr.cols, myhdr.nnz);

	CSRMatrix m;
	m.rows = myhdr.rows;
	m.cols = myhdr.cols;
	m.nnz = myhdr.nnz;

	m.rowPtr = malloc(sizeof(int)*(m.rows+1));
	m.col = malloc(sizeof(int)*m.nnz);
	m.val = malloc(sizeof(double)*m.nnz);
	
	MPI_Scatterv(bufCol, sendCountsOther, dispOther, MPI_INT, m.col, m.nnz, MPI_INT, 0,  MPI_COMM_WORLD);
	MPI_Scatterv(bufVal, sendCountsOther, dispOther, MPI_DOUBLE, m.val, m.nnz, MPI_DOUBLE, 0,  MPI_COMM_WORLD);
    MPI_Scatterv(bufPtr, sendCountsPtr, dispPtr, MPI_INT, m.rowPtr, m.rows+1, MPI_INT, 0,  MPI_COMM_WORLD);
	printf("====RANK %d====", world_rank);
	printCSR(&m);
    /*int value;
    if (world_rank == 0) value = 12345;        // only root sets the value
    MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    printf("Rank %d received broadcast value %d\n", world_rank, value);

    // Example: reduce (sum) values from all ranks to the root
    int my_val = world_rank;
    int sum = 0;
    MPI_Reduce(&my_val, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        printf("Sum of ranks = %d\n", sum);
    }*/

    MPI_Finalize();                             // Clean up MPI
    return 0;
}

