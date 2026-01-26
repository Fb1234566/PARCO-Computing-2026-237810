#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
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
        MPI_Init(&argc, &argv);                     // Initialize MPI

        int world_size, world_rank;
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);// number of processes
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);// this process' rank

        // --- Argument Parsing Defaults ---
        char *matrix_type = "file";
        char *file_path = "datasets/inline_1.mtx";
        char *export_path = "result/temp.csv";
        int syn_rows = 1000;
        int syn_cols = 1000;
        int syn_nnz = 5000;

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
            } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
                ex = argv[++i];
            } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
                export_path = argv[++i];
            }

        }

        // Create the data type to be sent via csr
        MPI_Datatype csr_header_type;
        create_csr_header_type(&csr_header_type);

        COOMatrix c1;
        CSRHeader* headers = NULL;
        CSRMatrix* procMatrices = NULL;
        Vector vector, finalRes, serialRes;
        int* bufPtr;
        int* bufCol;
        double* bufVal;
        Vector resVector;
        int sendCountsOther[world_size];
        int sendCountsV[world_size];
        int sendCountsPtr[world_size];
        int dispPtr[world_size];
        int dispOther[world_size];
        int dispV[world_size];
        int dispResV[world_size];

        // Start logger
        if (logger_init("app.log", LOG_LEVEL_INFO) != 0) {
                fprintf(stderr, "Unable to initialize logger\n");
                return 1;
        }

        LOG_INFO("=== Program start ===");

        if (world_rank == 0){
                CSRMatrix mComplete;
                procMatrices = malloc(sizeof(CSRMatrix)*world_size);
                headers = malloc(sizeof(CSRHeader)*world_size);
                COOMatrix* cooMatrices = malloc(sizeof(COOMatrix)*world_size);

                setRandSeed();

                if (strcmp(matrix_type, "synthetic") == 0) {
                    LOG_INFO("Generating synthetic matrix: %dx%d with %d NNZ", syn_rows, syn_cols, syn_nnz);
                    randomInitCOO(&c1, syn_rows, syn_cols, world_size, syn_nnz);
                } else {
                    LOG_INFO("Reading matrix from file: %s", file_path);
                    readMatrixCOO(file_path, &c1);
                }

                // Init the vector
                initVector(&vector, c1.cols);

                LOG_INFO("Computing correct result\n");
                // Compute correct result
                serialRes.len = c1.rows;
                serialRes.val = calloc(c1.rows, sizeof(double));
                COOToCSR(&c1, &mComplete);
                computeSpvmSerial(&mComplete, &vector, &serialRes);
                LOG_INFO("Computation done\n");

                // init the buffers
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
        Vector v;
        Vector res;
        m.rows = myhdr.rows;
        m.cols = myhdr.cols;
        m.nnz = myhdr.nnz;

        v.len = m.cols;
        v.val = malloc(sizeof(double)*m.cols);

        res.len = m.rows;
        res.val = calloc(m.rows, sizeof(double));

        m.rowPtr = malloc(sizeof(int)*(m.rows+1));
        m.col = malloc(sizeof(int)*m.nnz);
        m.val = malloc(sizeof(double)*m.nnz);

        MPI_Scatterv(bufCol, sendCountsOther, dispOther, MPI_INT, m.col, m.nnz, MPI_INT, 0,  MPI_COMM_WORLD);
        MPI_Scatterv(bufVal, sendCountsOther, dispOther, MPI_DOUBLE, m.val, m.nnz, MPI_DOUBLE, 0,  MPI_COMM_WORLD);
        MPI_Scatterv(bufPtr, sendCountsPtr, dispPtr, MPI_INT, m.rowPtr, m.rows+1, MPI_INT, 0,  MPI_COMM_WORLD);
        MPI_Scatterv(vector.val, sendCountsV, dispV, MPI_DOUBLE, v.val, v.len, MPI_DOUBLE, 0,  MPI_COMM_WORLD);

        // Computation
        LOG_INFO("Computing result for rank %d\n", world_rank);
        computeSpvmSerial(&m, &v, &res);
        LOG_INFO("Computation for rank %d done\n", world_rank);

        MPI_Gather(res.val, res.len, MPI_DOUBLE, resVector.val, res.len, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        free(m.rowPtr);
        free(m.col);
        free(m.val);
        free(v.val);
        free(res.val);

        if (world_rank == 0){
                int row;
                int currRank = 0;
                int localIdx = 0;

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
                } else {
                    LOG_ERROR("Result is incorrect");
                }

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
                Header h1;

            /* Initialize header with 3 columns */
            Header h;
            h.count = 3;
            h.s = malloc(sizeof(char*) * h.count);
            if (!h.s) { perror("malloc"); return 1; }
            h.s[0] = strdup("Time");
            h.s[1] = strdup("Iteration");
            h.s[2] = strdup("Status");

            /* Write header (only writes if file is empty) */
            appendToCSV(&h, NULL, export_path);

            /* Initialize values for one row */
            Values v;
            v.len = 3;
            v.value = malloc(sizeof(double) * v.len);
            if (!v.value) { perror("malloc"); return 1; }
            v.value[0] = 0.0;      /* Time */
            v.value[1] = 2.0;     /* Iteration */
            v.value[2] = 1.0;      /* Status as numeric */

            /* Append the values row */
            appendToCSV(NULL, &v, export_path);

            /* Free allocated memory */
            for (int i = 0; i < h.count; ++i) free(h.s[i]);
            free(h.s);
            free(v.value);

            printf("Wrote header and one row to %s\n", path);
            return 0;
        }

        MPI_Finalize();                     // Clean up MPI
        return 0;
}
