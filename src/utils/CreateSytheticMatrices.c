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
    if (logger_init("synthetic_matrix_generation.log", LOG_LEVEL_INFO) != 0) {
            fprintf(stderr, "Unable to initialize logger\n");
            return 1;
    }

    if (argc != 4) {
        fprintf(stderr, "Usage: %s rows cols nnz\n", argv[0]);
        return 1;
    }

    int rows, cols, nnz;
    if (parse_positive_int(argv[1], &rows) != 0 ||
        parse_positive_int(argv[2], &cols) != 0 ||
        parse_positive_int(argv[3], &nnz) != 0) {
        fprintf(stderr, "Error: rows, cols and nnz must be positive integers\n");
        return 1;
    }

    createWeakScalingDataset(rows, cols, nnz);

    char filename[256];
    if (snprintf(filename, sizeof(filename), "matrix_weak_scaling_%d_%d_%d.mtx", rows, cols, nnz) >= (int)sizeof(filename)) {
        fprintf(stderr, "Error: generated filename is too long\n");
        return 1;
    }

    if (access(filename, F_OK) == 0) {
        printf("Matrix written to %s\n", filename);
        return 0;
    } else {
        fprintf(stderr, "Failed to create %s\n", filename);
        return 1;
    }
}
