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
	printf("  --nprocs <int> Number of processros\n");
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
        MPI_Init(&argc, &argv);                     // Initialize MPI

        int world_size, world_rank;
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);// number of processes
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);// this process' rank

        // --- Argument Parsing Defaults ---
        char *matrix_type = "file";
        char *file_path = "datasets/inline_1.mtx";
        char *export_path = "results/temp.csv";
        int syn_rows = 1000;
        int syn_cols = 1000;
        int syn_nnz = 5000;
		int iteration = 0;
		int nprocs = -1;
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
            } else if (strcmp(argv[i], "--nprocs") == 0 && i + 1 < argc) {
                nprocs = atoi(argv[++i]);
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
        double* bufVal;
        Vector resVector;
		resVector.len = 0;
		resVector.val = NULL;
        int sendCountsOther[world_size];
        int sendCountsV[world_size];
        int sendCountsPtr[world_size];
        int dispPtr[world_size];
        int dispOther[world_size];
        int dispV[world_size];
        int dispResV[world_size];
		int reciveCountsResV[world_size];

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

                LOG_INFO("[RANK 0] Computing serial reference for %dx%d matrix", c1.rows, c1.cols);
                serialRes.len = c1.rows;
                serialRes.val = calloc(c1.rows, sizeof(double));
                COOToCSR(&c1, &mComplete);
                computeSpvmSerial(&mComplete, &vector, &serialRes);

                bufCol = calloc(c1.nnz, sizeof(int));
                bufPtr = calloc(c1.rows+1, sizeof(int));
                bufVal = calloc(c1.nnz, sizeof(double));
                resVector.len = c1.rows;
                resVector.val = calloc(c1.rows, sizeof(double));
                finalRes.len = c1.rows;
                finalRes.val = calloc(c1.rows, sizeof(double));
                splitCOOMatrix(&c1, cooMatrices, world_size);

                // Fill the headers in order to allow allocation of arrays
                int i;
                for(i=0; i<world_size; i++){
                        headers[i].rows = cooMatrices[i].rows;
                        headers[i].cols = cooMatrices[i].cols;
                        headers[i].nnz = cooMatrices[i].nnz;
                        if (i==0){
                                dispResV[i] = 0;
                        } else {
                                dispResV[i] = dispResV[i-1]+headers[i-1].rows;
                        }
                        reciveCountsResV[i] = headers[i].rows;
                }

                // Split the matrix into parts and convert them to CSR
                COOListToCSR(cooMatrices, procMatrices, world_size);
                for(int i = 0; i < world_size; i++){
                        free(cooMatrices[i].row);
                        free(cooMatrices[i].col);
                        free(cooMatrices[i].val);
                }

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

                        sendCountsV[i] = procMatrices[i].cols;
                        dispV[i] = 0;

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
                LOG_INFO("[RANK 0] Distributing matrix blocks to %d ranks", world_size);
        }

        // send the previously compiled headers
        CSRHeader myhdr;
        int* csrPtr, *csrCol;
        double* csrVal;
        MPI_Scatter(headers, 1, csr_header_type, &myhdr,  1, csr_header_type, 0, MPI_COMM_WORLD);

        CSRMatrix m;
        Vector res;
        m.rows = myhdr.rows;
        m.cols = myhdr.cols;
        m.nnz = myhdr.nnz;

		if (world_rank != 0){
			vector.len = m.cols;
			vector.val = calloc(m.cols, sizeof(double));
		}
        res.len = m.rows;
        res.val = calloc(m.rows, sizeof(double));

        m.rowPtr = malloc(sizeof(int)*(m.rows+1));
        m.col = malloc(sizeof(int)*m.nnz);
        m.val = malloc(sizeof(double)*m.nnz);

        MPI_Scatterv(bufCol, sendCountsOther, dispOther, MPI_INT, m.col, m.nnz, MPI_INT, 0,  MPI_COMM_WORLD);
        MPI_Scatterv(bufVal, sendCountsOther, dispOther, MPI_DOUBLE, m.val, m.nnz, MPI_DOUBLE, 0,  MPI_COMM_WORLD);
        MPI_Scatterv(bufPtr, sendCountsPtr, dispPtr, MPI_INT, m.rowPtr, m.rows+1, MPI_INT, 0,  MPI_COMM_WORLD);
        // MPI_Scatterv(vector.val, sendCountsV, dispV, MPI_DOUBLE, v.val, v.len, MPI_DOUBLE, 0,  MPI_COMM_WORLD);
		MPI_Bcast(vector.val, vector.len, MPI_DOUBLE, 0, MPI_COMM_WORLD);
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
    		resVector.val = calloc(c1.rows, sizeof(double));
    		gather_buffer = resVector.val;
    		gather_counts = reciveCountsResV;
    		gather_displs = dispResV;
		}

		MPI_Gatherv(res.val, res.len, MPI_DOUBLE, gather_buffer, gather_counts, gather_displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Reduce(&diff, &computeTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        free(m.rowPtr);
        free(m.col);
        free(m.val);
        free(vector.val);
        free(res.val);

        if (world_rank == 0){
                int row;
                int currRank = 0;
                int localIdx = 0;
				bool status = false;

                for(row=0; row<c1.rows; row++){
                        if (currRank + 1 < world_size && row >= dispResV[currRank + 1]){
                                currRank++;
                                localIdx = 0;
                        }
                        finalRes.val[currRank+localIdx*world_size] = resVector.val[row];
                        localIdx++;
                }

                if (compareVectors(&finalRes, &serialRes)){
                    LOG_INFO("Result is correct");
					status = true;
                } else {
                    LOG_ERROR("[RANK 0] Verification: FAILED");
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

            /* Initialize header with 3 columns */
            Header h;
            h.count = 4;
            h.s = malloc(sizeof(char*) * h.count);
            if (!h.s) { perror("malloc"); return 1; }
            h.s[0] = strdup("Time");
            h.s[1] = strdup("Iteration");
            h.s[2] = strdup("Status");
			h.s[3] = strdup("NProc");

            /* Write header (only writes if file is empty) */

            /* If export_path is a directory, build a file path inside it with the format:
               stats_mpi_SpMV_<sanitized>.csv
               where <sanitized> is either the sanitized file path (slashes -> underscores)
               or a synthetic description like synthetic_<rows>x<cols>_<nnz>.
            */
            char *final_export_path = export_path;
            bool allocated_path = false;
            struct stat st;
            if (stat(export_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                char sanitized[512];
                if (strcmp(matrix_type, "synthetic") == 0) {
                    snprintf(sanitized, sizeof(sanitized), "synthetic_%dx%d_%d", syn_rows, syn_cols, syn_nnz);
                } else {
                    /* Copy file_path and replace '/' with '_' */
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
            v.value[0] = computeTime;      /* Time */
            v.value[1] = iteration;     /* Iteration */
            v.value[2] = status;      /* Status as numeric */
			v.value[3] = nprocs;	/* number of processros */

            /* Append the values row */
            appendToCSV(NULL, &v, final_export_path);

            /* Free allocated memory */
            for (int i = 0; i < h.count; ++i) free(h.s[i]);
            free(h.s);
            free(v.value);
            if (allocated_path) free(final_export_path);

                LOG_INFO("[RANK 0] Completed in %.6f seconds", computeTime);
        }

        MPI_Finalize();                     // Clean up MPI
        return 0;
}
