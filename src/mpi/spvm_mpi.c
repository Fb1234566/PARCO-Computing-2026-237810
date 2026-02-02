#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "io.h"
#include "spvm_mpi.h"
#include "logger.h"
#include "exporter.h"
#include "spvm.h"

// --- Helper for CLI arguments ---
void print_help(char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("Options:\n");
    printf("  --type <str>    Matrix source type: 'file' or 'synthetic' (default: file)\n");
    printf("  --file <path>   Path to .mtx file (required if type is 'file')\n");
    printf("  --rows <int>    Number of rows (required if type is 'synthetic')\n");
    printf("  --cols <int>    Number of columns (required if type is 'synthetic')\n");
    printf("  --nnz  <int>    Number of non-zero elements (required if type is 'synthetic')\n");
    printf("  --export <path> Path to result file\n");
    printf("  --iteration <int> Path to result file\n");
	printf("  --help          Show this help message\n");
}

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
        MPI_Init(&argc, &argv);

        int world_size, world_rank;
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

        // --- Argument Parsing Defaults ---
        char *matrix_type = "file";
        char *file_path = "datasets/inline_1.mtx";
        char *export_path = "results/temp.csv";
        int syn_rows = 1000;
        int syn_cols = 1000;
        int syn_nnz = 5000;
		int iteration = 0;
        // Simple manual parsing
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--help") == 0) {
                if (world_rank == 0) print_help(argv[0]);
                MPI_Finalize();
                return 0;
            } else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
                matrix_type = argv[++i];
            } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
                file_path = argv[++i];
            } else if (strcmp(argv[i], "--rows") == 0 && i + 1 < argc) {
                syn_rows = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--cols") == 0 && i + 1 < argc) {
                syn_cols = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--nnz") == 0 && i + 1 < argc) {
                syn_nnz = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--export") == 0 && i + 1 < argc) {
                export_path = argv[++i];
            } else if (strcmp(argv[i], "--iteration") == 0 && i + 1 < argc) {
                iteration = atoi(argv[++i]);
            }
        }

        // Create the data type to be sent via csr
        MPI_Datatype csr_header_type;
        create_csr_header_type(&csr_header_type);

        COOMatrix c1;
		double computeTime;
        CSRHeader* headers = NULL;
        CSRMatrix* procMatrices = NULL;
        Vector vector, finalRes, serialRes;
		vector.len = 0;
		vector.val = NULL;
        int* bufPtr = NULL;
        int* bufCol = NULL;
        double* bufVal = NULL;
        Vector resVector;
		resVector.len = 0;
		resVector.val = NULL;


        // Dynamically allocate arrays to avoid stack overflow with large world_size
		int* sendCountsOther = malloc(world_size * sizeof(int));
		int* sendCountsV = malloc(world_size * sizeof(int));
		int* sendCountsPtr = malloc(world_size * sizeof(int));
		int* dispPtr = malloc(world_size * sizeof(int));
		int* dispOther = malloc(world_size * sizeof(int));
		int* dispV = malloc(world_size * sizeof(int));
		int* dispResV = malloc(world_size * sizeof(int));
		int* reciveCountsResV = malloc(world_size * sizeof(int));

		// Initialize to zero on all ranks
		memset(sendCountsOther, 0, world_size * sizeof(int));
		memset(sendCountsV, 0, world_size * sizeof(int));
		memset(sendCountsPtr, 0, world_size * sizeof(int));
		memset(dispPtr, 0, world_size * sizeof(int));
		memset(dispOther, 0, world_size * sizeof(int));
		memset(dispV, 0, world_size * sizeof(int));
		memset(dispResV, 0, world_size * sizeof(int));
		memset(reciveCountsResV, 0, world_size * sizeof(int));

        // Start logger
        if (logger_init("app.log", LOG_LEVEL_INFO) != 0) {
                fprintf(stderr, "Unable to initialize logger\n");
                return 1;
        }

        if (world_rank == 0){
                LOG_INFO("[RANK 0] Start with %d processes", world_size);
                CSRMatrix mComplete;
                procMatrices = malloc(sizeof(CSRMatrix)*world_size);
                headers = malloc(sizeof(CSRHeader)*world_size);
                COOMatrix* cooMatrices = malloc(sizeof(COOMatrix)*world_size);

                setRandSeed();

                if (strcmp(matrix_type, "synthetic") == 0) {
                    LOG_INFO("[RANK 0] Matrix: synthetic %dx%d, nnz=%d", syn_rows, syn_cols, syn_nnz);
                    randomInitCOO(&c1, syn_rows, syn_cols, world_size, syn_nnz);
                } else {
                    LOG_INFO("[RANK 0] Matrix: file %s", file_path);
                    readMatrixCOO(file_path, &c1);
                }

                initVector(&vector, c1.cols);
				LOG_INFO("[RANK 0] Vector initialized, first 5 values: %.6f, %.6f, %.6f, %.6f, %.6f",
        			vector.val[0], vector.val[1], vector.val[2], vector.val[3], vector.val[4]);

                LOG_INFO("[RANK 0] Computing serial reference for %dx%d matrix", c1.rows, c1.cols);
                serialRes.len = c1.rows;
                serialRes.val = calloc(c1.rows, sizeof(double));
                COOToCSR(&c1, &mComplete);
                computeSpvmSerial(&mComplete, &vector, &serialRes);

                // Free mComplete
                free(mComplete.rowPtr);
                free(mComplete.col);
                free(mComplete.val);

                bufCol = calloc(c1.nnz, sizeof(int));
                bufVal = calloc(c1.nnz, sizeof(double));
                resVector.len = c1.rows;
                resVector.val = calloc(c1.rows, sizeof(double));
                finalRes.len = c1.rows;
                finalRes.val = calloc(c1.rows, sizeof(double));

                splitCOOMatrix(&c1, cooMatrices, world_size);
				LOG_INFO("[RANK 0] MATRIX 0, first 5 values: %.6f, %.6f, %.6f, %.6f, %.6f",
        			cooMatrices[0].val[0], cooMatrices[0].val[1], cooMatrices[0].val[2], cooMatrices[0].val[3], cooMatrices[0].val[4]);

                // Split the matrix into parts and convert them to CSR
                COOListToCSR(cooMatrices, procMatrices, world_size);

                // Calculate total buffer size needed for rowPtr (sum of all procMatrices[i].rows+1)
                int total_rowptr_size = 0;
                int i;
                for(i=0; i<world_size; i++){
                        total_rowptr_size += procMatrices[i].rows + 1;
                }

                // Allocate bufPtr with correct size
                bufPtr = calloc(total_rowptr_size, sizeof(int));

                // Fill the headers in order to allow allocation of arrays
                for(i=0; i<world_size; i++){
                        headers[i].rows = cooMatrices[i].rows;
                        headers[i].cols = cooMatrices[i].cols;
                        headers[i].nnz = cooMatrices[i].nnz;
                        if (i==0){
                                dispResV[i] = 0;
                                LOG_INFO("[RANK 0] headers[0]: rows=%d, cols=%d, nnz=%d",
                                        headers[0].rows, headers[0].cols, headers[0].nnz);
                        } else {
                                dispResV[i] = dispResV[i-1]+headers[i-1].rows;
                        }
                        reciveCountsResV[i] = headers[i].rows;
                }
                for(int i = 0; i < world_size; i++){
                        free(cooMatrices[i].row);
                        free(cooMatrices[i].col);
                        free(cooMatrices[i].val);
                }

                free(cooMatrices);
                int offsetOther = 0;
                int offsetPtr = 0;

                // Prepare the buffers, send counts and offsets in order to send the data via scatterv
                for(i=0; i<world_size; i++){
                        sendCountsOther[i] = procMatrices[i].nnz;
                        dispOther[i] = offsetOther;
                        offsetOther += sendCountsOther[i];

                        sendCountsPtr[i] = procMatrices[i].rows+1;
                        dispPtr[i] = offsetPtr;
                        offsetPtr += sendCountsPtr[i];

                        sendCountsV[i] = procMatrices[i].cols;
                        dispV[i] = 0;

                        // Column index
                        if (sendCountsOther[i] > 0 && procMatrices[i].col != NULL) {
                                memcpy(bufCol + dispOther[i], procMatrices[i].col,
                                       (size_t)sendCountsOther[i] * sizeof(bufCol[0]));
                        }

                        // Values
                        if (sendCountsOther[i] > 0 && procMatrices[i].val != NULL) {
                                memcpy(bufVal + dispOther[i], procMatrices[i].val,
                                       (size_t)sendCountsOther[i] * sizeof(bufVal[0]));
                        }

                        // Row pointers
                        if (sendCountsPtr[i] > 0 && procMatrices[i].rowPtr != NULL) {
                                memcpy(bufPtr + dispPtr[i], procMatrices[i].rowPtr,
                                       (size_t)sendCountsPtr[i] * sizeof(bufPtr[0]));
                        }
                }

                LOG_INFO("[RANK 0] Distributing matrix blocks to %d ranks", world_size);
        }

        // Send the previously compiled headers
        CSRHeader myhdr;
        MPI_Scatter(headers, 1, csr_header_type, &myhdr, 1, csr_header_type, 0, MPI_COMM_WORLD);

        CSRMatrix m;
        Vector res;
        m.rows = myhdr.rows;
        m.cols = myhdr.cols;
        m.nnz = myhdr.nnz;

        // Broadcast vector length first
        int vec_len;
        if (world_rank == 0) {
            vec_len = vector.len;
        }
        MPI_Bcast(&vec_len, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Allocate vector on non-root ranks (ensure minimum size 1)
        if (world_rank != 0) {
            vector.len = vec_len;
            vector.val = malloc(sizeof(double) * ((vec_len > 0) ? vec_len : 1));
        }

        res.len = m.rows;
        res.val = calloc((m.rows > 0) ? m.rows : 1, sizeof(double));

        // Ensure minimum allocation size of 1 to avoid malloc(0)
        m.rowPtr = malloc(sizeof(int) * ((m.rows + 1 > 0) ? (m.rows + 1) : 1));
        m.col = malloc(sizeof(int) * ((m.nnz > 0) ? m.nnz : 1));
        m.val = malloc(sizeof(double) * ((m.nnz > 0) ? m.nnz : 1));

        MPI_Scatterv(bufCol, sendCountsOther, dispOther, MPI_INT, m.col, m.nnz, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Scatterv(bufVal, sendCountsOther, dispOther, MPI_DOUBLE, m.val, m.nnz, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Scatterv(bufPtr, sendCountsPtr, dispPtr, MPI_INT, m.rowPtr, m.rows+1, MPI_INT, 0, MPI_COMM_WORLD);


        // Broadcast the full vector to all ranks
        MPI_Bcast(vector.val, vec_len, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		LOG_INFO("[RANK %d] Vector received, first 5 values: %.6f, %.6f, %.6f, %.6f, %.6f",
        	world_rank, vector.val[0], vector.val[1], vector.val[2], vector.val[3], vector.val[4]);
   		LOG_INFO("[RANK %d] Matrix received, first 5 values: %.6f, %.6f, %.6f, %.6f, %.6f",
        	world_rank, m.val[0], m.val[1], m.val[2], m.val[3], m.val[4]);
        LOG_INFO("[RANK %d] Received matrix block: %dx%d, nnz=%d", world_rank, m.rows, m.cols, m.nnz);

        double start, end;
        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();
        computeSpvmSerial(&m, &vector, &res);
        end = MPI_Wtime();

        double diff = end - start;
        LOG_INFO("[RANK %d] Computation completed in %.6f seconds", world_rank, diff);

        double* gather_buffer = NULL;
        int* gather_counts = NULL;
        int* gather_displs = NULL;

		if (world_rank == 0) {
    		gather_buffer = resVector.val;
    		gather_counts = reciveCountsResV;
    		gather_displs = dispResV;
		}
		LOG_INFO("[RANK %d] Vecto, first 5 values: %.6f, %.6f, %.6f, %.6f, %.6f",
        	world_rank, res.val[0], res.val[1], res.val[2], res.val[3], res.val[4]);

		MPI_Gatherv(res.val, res.len, MPI_DOUBLE, gather_buffer, gather_counts, gather_displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Reduce(&diff, &computeTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        free(m.rowPtr);
        free(m.col);
        free(m.val);
        if (world_rank != 0) {
            free(vector.val);
        }
        free(res.val);

        if (world_rank == 0){
                bool status = false;

                // Reconstruct the result in original row order
                for (int rank = 0; rank < world_size; rank++) {
                    int startIdx = dispResV[rank];
                    int count = reciveCountsResV[rank];

                    for (int j = 0; j < count; j++) {
                        int originalRow = rank + j * world_size;
                        if (originalRow < c1.rows) {
                            finalRes.val[originalRow] = resVector.val[startIdx + j];
                        }
                    }
                }

                if (compareVectors(&finalRes, &serialRes)){
                    LOG_INFO("Result is correct");
					status = true;
                } else {
                    LOG_ERROR("[RANK 0] Verification: FAILED");
					    // Show first few mismatches
    				int mismatch_count = 0;
    				for (int i = 0; i < c1.rows && mismatch_count < 10; i++) {
        				if (finalRes.val[i] != serialRes.val[i]) {
            				LOG_ERROR("[RANK 0] Mismatch at row %d: got %.10f, expected %.10f",
                      		i, finalRes.val[i], serialRes.val[i]);
            				mismatch_count++;
        				}
    				}
                }

				free(c1.row);
				free(c1.col);
				free(c1.val);
				free(serialRes.val);

                for(int i = 0; i < world_size; i++){
                        free(procMatrices[i].rowPtr);
                        free(procMatrices[i].col);
                        free(procMatrices[i].val);
                }
                free(procMatrices);

                free(bufCol);
                free(bufPtr);
				free(bufVal);
                free(headers);
                free(vector.val);
                free(resVector.val);
                free(finalRes.val);

            /* Initialize header with 4 columns */
            Header h;
            h.count = 4;
            h.s = malloc(sizeof(char*) * h.count);
            if (!h.s) { perror("malloc"); return 1; }

            h.s[0] = strdup("Time");
            h.s[1] = strdup("Iteration");
            h.s[2] = strdup("Status");
			h.s[3] = strdup("NProc");

            /* If export_path is a directory, build a file path inside it */
            char *final_export_path = export_path;
            bool allocated_path = false;
            struct stat st;
            if (stat(export_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                char sanitized[512];
                if (strcmp(matrix_type, "synthetic") == 0) {
                    snprintf(sanitized, sizeof(sanitized), "synthetic_%dx%d_%d", syn_rows, syn_cols, syn_nnz);
                } else {
                    snprintf(sanitized, sizeof(sanitized), "%s", file_path);
                    for (char *p = sanitized; *p; ++p) if (*p == '/') *p = '_';
                }
                size_t need = strlen(export_path) + 1 + strlen("stats_mpi_SpMV_") + strlen(sanitized) + strlen(".csv") + 1;
                final_export_path = malloc(need);
                if (!final_export_path) { perror("malloc"); return 1; }
                allocated_path = true;
                if (export_path[strlen(export_path)-1] == '/') {
                    snprintf(final_export_path, need, "%sstats_mpi_SpMV_%s.csv", export_path, sanitized);
                } else {
                    snprintf(final_export_path, need, "%s/stats_mpi_SpMV_%s.csv", export_path, sanitized);
                }
            }

            appendToCSV(&h, NULL, final_export_path);

            /* Initialize values for one row */
            Values v;
            v.len = 4;
            v.value = malloc(sizeof(double) * v.len);
            if (!v.value) { perror("malloc"); return 1; }
            v.value[0] = computeTime;
            v.value[1] = iteration;
            v.value[2] = status;
			v.value[3] = world_size;

            appendToCSV(NULL, &v, final_export_path);

            /* Free allocated memory */
            for (int i = 0; i < h.count; ++i) {
                free(h.s[i]);
            }
            free(h.s);
            free(v.value);
            if (allocated_path) {
                free(final_export_path);
            }

            LOG_INFO("[RANK 0] Completed in %.6f seconds", computeTime);
        }

        // Free dynamically allocated arrays (ALL RANKS)
        free(sendCountsOther);
        free(sendCountsV);
        free(sendCountsPtr);
        free(dispPtr);
        free(dispOther);
        free(dispV);
        free(dispResV);
        free(reciveCountsResV);

        MPI_Finalize();
        return 0;
}