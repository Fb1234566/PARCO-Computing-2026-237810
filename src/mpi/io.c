#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "logger.h"
#include <stdio.h>

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
    if (!m) return;
    for (int i = 0; i < m->nnz; ++i) {
        printf("COO [%d]: row=%d col=%d val=%f\n", i, m->row[i], m->col[i], m->val[i]);
    }
}

void printCSR(const CSRMatrix* m){
    if (!m) return;
    if (!m->rowPtr || !m->col || !m->val) return;

    for (int r = 0; r < m->rows; ++r) {
        int start = m->rowPtr[r];
        int end = (r + 1 <= m->rows) ? m->rowPtr[r + 1] : m->nnz;
        if (start < 0) start = 0;
        if (end > m->nnz) end = m->nnz;
        if (start >= end) continue;

        for (int idx = start; idx < end; ++idx) {
            printf("CSR row=%d idx=%d: col=%d val=%f\n", r, idx, m->col[idx], m->val[idx]);
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

void randomInitCOO(CSRMatrix* m, int rows, int cols, int nRanks, int nnz){
    LOG_INFO("Start randomInitCOO: rows=%d cols=%d nRanks=%d requested_nnz=%d", rows, cols, nRanks, nnz);

    if (nnz <= 0 || rows <= 0 || cols <= 0 || nRanks <= 0) {
        LOG_ERROR("Invalid parameters to randomInitCOO");
        return;
    }

    COOEntry* elements = malloc(sizeof(COOEntry) * (size_t)nnz);
    if (!elements) {
        LOG_ERROR("Memory allocation failed for elements (nnz=%d)", nnz);
        return;
    }
    LOG_INFO("Allocated elements buffer for %d entries", nnz);

    int nnzPerRank = nnz / nRanks;
    int currRank;
    int rowsPerRank = rows / nRanks;
    int currNumOfElems = 0;

    for (currRank = 0; currRank < nRanks; currRank++) {
        int i;
        for (i = 0; i < nnzPerRank; i++) {
            COOEntry elem;
            do {
                elem.row = generateRandInt(currRank * rowsPerRank, currRank * rowsPerRank + rowsPerRank);
                elem.col = generateRandInt(0, cols - 1);
                elem.val = generateRandDouble(0.0, 10000.0);
            } while (checkIfValueIsAlreadyPresent(elements, &elem, currNumOfElems));
            elements[currNumOfElems] = elem;
            currNumOfElems++;
        }
    }

    LOG_INFO("Generated %d unique COO entries (before sort)", currNumOfElems);

    qsort(elements, (size_t)currNumOfElems, sizeof(COOEntry), COOEntryCompartor);
    LOG_INFO("Sorted %d COO entries", currNumOfElems);

    COOMatrix temp;
    temp.rows = rows;
    temp.cols = cols;
    temp.nnz = currNumOfElems;
    temp.row = calloc((size_t)temp.nnz, sizeof(int));
    temp.col = calloc((size_t)temp.nnz, sizeof(int));
    temp.val = calloc((size_t)temp.nnz, sizeof(double));
    if (!temp.row || !temp.col || !temp.val) {
        LOG_ERROR("Memory allocation failed for temporary COOMatrix (nnz=%d)", temp.nnz);
        free(elements);
        free(temp.row); free(temp.col); free(temp.val);
        return;
    }
    LOG_INFO("Allocated temporary COO arrays for %d entries", temp.nnz);

    for (int i = 0; i < temp.nnz; i++) {
        temp.row[i] = elements[i].row;
        temp.col[i] = elements[i].col;
        temp.val[i] = elements[i].val;
    }

    free(elements);
    LOG_INFO("Converted generated entries into temporary COOMatrix, calling COOToCSR");

    COOToCSR(&temp, m);
    LOG_INFO("COOToCSR completed");

    free(temp.row);
    free(temp.col);
    free(temp.val);
    LOG_INFO("Finished randomInitCOO");
}

int generateRandInt(int min, int max){
    if (max <= min) return min;
    return min + (rand() % (max - min + 1));
}

bool checkIfValueIsAlreadyPresent(COOEntry* c, COOEntry* e, int n){
    int i;
    for (i = 0; i < n; i++) {
        if (c[i].row == e->row && c[i].col == e->col) {
            return true;
        }
    }
    return false;
}

double generateRandDouble(double lower_bound, double upper_bound){
    if (upper_bound <= lower_bound) return lower_bound;
    return lower_bound + ((double)rand() / (double)RAND_MAX) * (upper_bound - lower_bound);
}

void setRandSeed(){
    srand((unsigned)time(NULL));
    LOG_INFO("Random seed set with time(NULL)");
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

	CSRMatrix c1;
	randomInitCOO(&c1, 10, 10, 5, 50);
	printCSR(&c1);

    logger_close();
    LOG_INFO("=== Program end ===");
    return 0;
}