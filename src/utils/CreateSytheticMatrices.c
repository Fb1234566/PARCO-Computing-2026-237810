#include "DataFactory.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "../mpi/logger.h"

static int parse_positive_int(const char *s, int *out) {
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return -1;
    if (v <= 0 || v > INT_MAX) return -1;
    *out = (int)v;
    return 0;
}

int main(int argc, char **argv) {
    if (logger_init("../synthetic_matrix_generation.log", LOG_LEVEL_INFO) != 0) {
            fprintf(stderr, "Unable to initialize logger\n");
            return 1;
    }

    LOG_INFO("Starting synthetic matrix generation");

    if (argc != 4) {
        LOG_ERROR("Invalid arguments: expected 3 arguments (rows, cols, nnz), got %d", argc - 1);
        fprintf(stderr, "Usage: %s rows cols nnz\n", argv[0]);
        logger_close();
        return 1;
    }

    int rows, cols, nnz;
    if (parse_positive_int(argv[1], &rows) != 0 ||
        parse_positive_int(argv[2], &cols) != 0 ||
        parse_positive_int(argv[3], &nnz) != 0) {
        LOG_ERROR("Invalid parameters: rows, cols, and nnz must be positive integers");
        fprintf(stderr, "Error: rows, cols and nnz must be positive integers\n");
        logger_close();
        return 1;
    }

    LOG_INFO("Generating matrix: rows=%d, cols=%d, nnz=%d", rows, cols, nnz);
    createWeakScalingDataset(rows, cols, nnz);

    char filename[256];
    if (snprintf(filename, sizeof(filename), "matrix_weak_scaling_%d_%d_%d.mtx", rows, cols, nnz) >= (int)sizeof(filename)) {
        LOG_ERROR("Filename too long for matrix %dx%d with %d non-zeros", rows, cols, nnz);
        fprintf(stderr, "Error: generated filename is too long\n");
        logger_close();
        return 1;
    }

    if (access(filename, F_OK) == 0) {
        LOG_INFO("Successfully created matrix file: %s", filename);
        printf("Matrix written to %s\n", filename);
        logger_close();
        return 0;
    } else {
        LOG_ERROR("Failed to create matrix file: %s", filename);
        fprintf(stderr, "Failed to create %s\n", filename);
        logger_close();
        return 1;
    }
}
