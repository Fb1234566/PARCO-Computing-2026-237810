#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "logger.h"

void readMatrixCOO(const char* path, COOMatrix* m){
    LOG_INFO("Start reading COO matrix from '%s'", path);
    FILE *f = fopen(path, "r");
    if (!f) {
        LOG_ERROR("Unable to open file '%s'", path);
        return;
    }

    // parse the comments for the dimensions of the matrix
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
	COOEntry* cooList = NULL;

    while ((nread = getline(&line, &len, f)) != -1) {
        if (nread > 0 && line[nread-1] == '\n') line[nread-1] = '\0';

        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;    // skip leading spaces
        if (*p == '%' || *p == '\0') continue;            // skip comments / empty lines

        if (sscanf(p, "%d %d %d", &m->rows, &m->cols, &m->nnz) == 3) {
            LOG_INFO("Dimensions: %dx%d, nnz=%d", m->rows, m->cols, m->nnz);
            break;
        } else {
            LOG_ERROR("Malformed dimensions line: %s", p);
            free(line);
            fclose(f);
            return;
        }
    }

    m->row = calloc(m->nnz, sizeof(int));
    m->col = calloc(m->nnz, sizeof(int));
    m->val = calloc(m->nnz, sizeof(double));
	cooList = malloc(m->nnz * sizeof(COOEntry));

	int lineIdx = 0;
    if (!m->row || !m->col || !m->val || !cooList) {
        LOG_ERROR("Memory allocation failed for COO");
        free(m->row); free(m->col); free(m->val);
        free(cooList);
        free(line);
        fclose(f);
        return;
    }

    while ((nread = getline(&line, &len, f)) != -1) {
        if (nread > 0 && line[nread-1] == '\n') line[nread-1] = '\0';

        int row, col;
        double val;

        if (sscanf(line, "%d %d %lf", &row, &col, &val) == 3) {
			COOEntry e;
			e.row = row-1;
			e.col = col-1;
			e.val = val;
			cooList[lineIdx] = e;
        } else {
            LOG_ERROR("Malformed data line: %s", line);
            free(cooList);
            free(line);
            fclose(f);
			return;
        }
		lineIdx++;
    }
	qsort(cooList, m->nnz, sizeof(COOEntry), COOEntryCompartor);

	int i;
	for (i=0; i<m->nnz; i++){
		m->row[i] = cooList[i].row;
		m->col[i] = cooList[i].col;
		m->val[i] = cooList[i].val;
	}

    free(cooList);
    free(line);
    fclose(f);
    LOG_INFO("Finished reading COO matrix from '%s'", path);
}

void COOToCSR(COOMatrix* in, CSRMatrix* out){
    LOG_INFO("Start converting COO to CSR");
	int currRow = -1;
	int idx=0;
	int i = 0;
	out->rows = in->rows;
	out->cols = in->cols;
	out->nnz = in->nnz;
	out->rowPtr = calloc(out->rows +1, sizeof(int));
	out->col = calloc(out->nnz, sizeof(int));
	out->val = calloc(out->nnz, sizeof(double));
	if (!out->rowPtr || !out->col || !out->val) {
    	LOG_ERROR("Memory allocation failed for CSR");
    	free(out->rowPtr); free(out->col); free(out->val);
    	return;
	}
	for(i=0; i<in->nnz; i++){
		if (currRow != in->row[i]){
			int j = 0;
			for(j=0; j<(in->row[i] - currRow); j++){
				out->rowPtr[idx] = i;
				idx++;
			}
			currRow = in->row[i];

		}

	    out->col[i] = in->col[i];
		out->val[i] = in->val[i];

	}

	for (; idx <= out->rows; ++idx) {
		out->rowPtr[idx] = out->nnz;
	}
    LOG_INFO("Finished converting COO to CSR");
}

void printCOO(const COOMatrix* m){
    // Removed noisy logs inside print functions
    if (!m) return;
    for (int i = 0; i < m->nnz; ++i) {
        // printing suppressed; keep function available if needed for debug
    }
}

void printCSR(const CSRMatrix* m){
    // Removed noisy logs inside print functions
    if (!m) return;
    if (!m->rowPtr || !m->col || !m->val) return;

    for (int r = 0; r < m->rows; ++r) {
        int start = m->rowPtr[r];
        int end = (r + 1 <= m->rows) ? m->rowPtr[r + 1] : m->nnz;
        if (start < 0) start = 0;
        if (end > m->nnz) end = m->nnz;
        if (start >= end) continue;

        for (int idx = start; idx < end; ++idx) {
            // printing suppressed; keep function available if needed for debug
        }
    }
}

int COOEntryCompartor(const void* a, const void* b){
	const COOEntry* x = (const COOEntry*)a;
	const COOEntry* y = (const COOEntry*)b;

	if (x->row < y->row) return -1;
	if (x->row > y->row) return 1;
	if (x->col < y->col) return -1;
	if (x->col > y->col) return 1;
	return 0;

}

int main() {
    LOG_INFO("=== Program start ===");
	if (logger_init("app.log", LOG_LEVEL_INFO) != 0) {
		fprintf(stderr, "Unable to initialize logger\n");
		return 1;
	}

    LOG_INFO("Step: Read matrix (COO)");
	COOMatrix m;
    readMatrixCOO("datasets/inline_1.mtx", &m);
    LOG_INFO("Step completed: Read matrix (COO)");

    LOG_INFO("Step: Convert to CSR");
	CSRMatrix c;
	COOToCSR(&m,&c);
    LOG_INFO("Step completed: Convert to CSR");

    logger_close();
    LOG_INFO("=== Program end ===");
    return 0;
}