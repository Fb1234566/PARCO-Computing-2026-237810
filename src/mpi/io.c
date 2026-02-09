#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include "logger.h"
#include <stdio.h>

// Funzione hash per combinare (row, col) in una chiave unica a 64 bit
static unsigned long hashCoordinate(int row, int col) {
    return ((unsigned long)row << 32) | (unsigned long)col;
}

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
    size_t nread;
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

void randomInitCOO(COOMatrix* m, int rows, int cols, int nRanks, int nnz){
    LOG_INFO("Start randomInitCOO: rows=%d cols=%d nRanks=%d requested_nnz=%d", rows, cols, nRanks, nnz);

    if (nnz <= 0 || rows <= 0 || cols <= 0 || nRanks <= 0) {
        LOG_ERROR("Invalid parameters to randomInitCOO");
        return;
    }

	if (nnz < rows){
		LOG_WARN("Number of nnz is lower than the number of rows. This causes epmty lines and unbalanced matrices");
	}

    int hashSize = nnz * 2;
    bool* used = calloc(hashSize, sizeof(bool));
    unsigned long* keys = malloc(hashSize * sizeof(unsigned long));

    if (!used || !keys) {
        LOG_ERROR("Memory allocation failed for hash table");
        free(used);
        free(keys);
        return;
    }
    LOG_INFO("Allocated hash table with size %d", hashSize);

    COOEntry* elements = malloc(sizeof(COOEntry) * (size_t)nnz);
    if (!elements) {
        LOG_ERROR("Memory allocation failed for elements (nnz=%d)", nnz);
        free(used);
        free(keys);
        return;
    }
    LOG_INFO("Allocated elements buffer for %d entries", nnz);

	int insertedNNZ = 0;
	int currRow = 0;
	int attempts = 0;
	int maxAttemptsPerElement = cols * 2;

	while (insertedNNZ < nnz){
        int row = currRow % rows;
        int col;
        bool inserted = false;

        for (int attempt = 0; attempt < maxAttemptsPerElement && !inserted; attempt++) {
            col = generateRandInt(0, cols - 1);
            unsigned long key = hashCoordinate(row, col);
            unsigned long hash = key % hashSize;

            bool isDuplicate = false;
            for (int i = 0; i < hashSize; i++) {
                unsigned long pos = (hash + i) % hashSize;

                if (!used[pos]) {
                    used[pos] = true;
                    keys[pos] = key;
                    elements[insertedNNZ].row = row;
                    elements[insertedNNZ].col = col;
                    elements[insertedNNZ].val = generateRandDouble(-100.0, 100.0);
                    insertedNNZ++;
                    currRow++;
                    inserted = true;
                    break;
                } else if (keys[pos] == key) {
                    isDuplicate = true;
                    attempts++;
                    break;
                }
            }

            if (inserted) break;
        }

        if (!inserted) {
            LOG_WARN("Unable to insert element for row %d after %d attempts - matrix may be too dense", row, maxAttemptsPerElement);
            break;
        }
	}

    free(used);
    free(keys);

    if (insertedNNZ < nnz) {
        LOG_WARN("Generated only %d unique entries out of %d requested after %d attempts", insertedNNZ, nnz, attempts);
    }

    LOG_INFO("Generated %d unique COO entries after %d attempts (before sort)", insertedNNZ, attempts);

    qsort(elements, (size_t)insertedNNZ, sizeof(COOEntry), COOEntryCompartor);
    LOG_INFO("Sorted %d COO entries", insertedNNZ);

    m->rows = rows;
    m->cols = cols;
    m->nnz = insertedNNZ;
    m->row = calloc((size_t)m->nnz, sizeof(int));
    m->col = calloc((size_t)m->nnz, sizeof(int));
    m->val = calloc((size_t)m->nnz, sizeof(double));
    if (!m->row || !m->col || !m->val) {
        LOG_ERROR("Memory allocation failed for temporary COOMatrix (nnz=%d)", m->nnz);
        free(elements);
        free(m->row); free(m->col); free(m->val);
        return;
    }
    LOG_INFO("Allocated COO arrays for %d entries", m->nnz);

    for (int i = 0; i < m->nnz; i++) {
        m->row[i] = elements[i].row;
        m->col[i] = elements[i].col;
        m->val[i] = elements[i].val;
    }

    free(elements);
    LOG_INFO("Converted generated entries into COOMatrix");
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

void splitCOOMatrix(COOMatrix* inMatrix, COOMatrix* arrayMatrices, const int P){
	LOG_INFO("splitCOOMatrix: start rows=%d cols=%d nnz=%d P=%d", inMatrix->rows, inMatrix->cols, inMatrix->nnz, P);

	int i;
	int owner;
	int* ownerIdx = calloc(P, sizeof(int));
	int* valuesPerRow = calloc(P, sizeof(int));
	int* rowsPerOwner = calloc(P, sizeof(int));
	int* currRowPerOwner = calloc(P, sizeof(int));
	int* lastRowPerOwner = calloc(P, sizeof(int));
	LOG_INFO("splitCOOMatrix: allocated ownerIdx=%p valuesPerRow=%p for P=%d", (void*)ownerIdx, (void*)valuesPerRow, P);

	// Initialize all last row per owner to -1
	for (i=0; i<P; i++){
		lastRowPerOwner[i] = -1;
		currRowPerOwner[i] = -1;
	}

	for(i=0; i<inMatrix->nnz; i++) {
		owner = inMatrix->row[i]%P;
		valuesPerRow[owner]++;
        if (inMatrix->row[i] != lastRowPerOwner[owner]) {
            rowsPerOwner[owner]++;
            lastRowPerOwner[owner] = inMatrix->row[i];
        }
	}

	for(i=0; i<P; i++){
		LOG_INFO("splitCOOMatrix: owner %d will receive %d entries", i, valuesPerRow[i]);
	}

	// Initialize array of matrices
	for (i = 0; i < P; i++){
		arrayMatrices[i].rows = rowsPerOwner[i];
		arrayMatrices[i].cols = inMatrix->cols;
		arrayMatrices[i].nnz = valuesPerRow[i];
		arrayMatrices[i].row = calloc(valuesPerRow[i], sizeof(int));
		arrayMatrices[i].col = calloc(valuesPerRow[i], sizeof(int));
		arrayMatrices[i].val = calloc(valuesPerRow[i], sizeof(double));}

	for (i=0; i<P; i++){
		lastRowPerOwner[i] = -1;
	}
	//slice the matrix
	for (i = 0; i < inMatrix->nnz; i++){
		owner = inMatrix->row[i]%P;
		int* currIdx = &ownerIdx[owner];

		if (inMatrix->row[i] != lastRowPerOwner[owner]) {
			++currRowPerOwner[owner];
			lastRowPerOwner[owner] = inMatrix->row[i];
		}
		arrayMatrices[owner].row[*currIdx] = currRowPerOwner[owner];
		arrayMatrices[owner].col[*currIdx] = inMatrix->col[i];
		arrayMatrices[owner].val[*currIdx] = inMatrix->val[i];

		(*currIdx)++;
	}

	for (i = 0; i < P; i++){
		LOG_INFO("splitCOOMatrix: owner %d received %d entries (expected %d)", i, ownerIdx[i], valuesPerRow[i]);
	}

	LOG_INFO("splitCOOMatrix: end");
}



void initCOO(COOMatrix* m, const int rows, const int cols, const int nnz){
	m->rows = rows;
	m->cols = cols;
	m->nnz = nnz;
	m->row = calloc(nnz, sizeof(int));
	m->col = calloc(nnz, sizeof(int));
	m->val = calloc(nnz, sizeof(double));
}

void COOListToCSR(COOMatrix* in, CSRMatrix* out, const int P){
	int i;
	for(i = 0; i<P; i++){
		COOToCSR(&in[i], &out[i]);
	}
}

void initVector(Vector* v, int len){
	v->len = len;
	v->val = calloc(len, sizeof(double));
	int i;
	for(i=0; i<len; i++){
		v->val[i] = generateRandDouble(-100.0, 100.0);
	}
}

void printVector(const Vector* v){
	int i;
	for(i=0; i<v->len; i++){
		printf("idx: %d, val: %f\n", i, v->val[i]);
	}
}

bool compareVectors(Vector* v1, Vector* v2){
	if (v1->len != v2->len){
		LOG_ERROR("Icompatible sizes for comparison: %d and %d\n", v1->len, v2->len);
		return false;
	}
	
	int i;
	for (i=0; i<v1->len; i++){
		if (v1->val[i] != v2->val[i]) return false;
	}
	return true;
}
		

/*int main() {
    LOG_INFO("=== Program start ===");
	if (logger_init("app.log", LOG_LEVEL_INFO) != 0) {
		fprintf(stderr, "Unable to initialize logger\n");
		return 1;
	}
	Vector v;
	initVector(&v, 15);
	printVector(&v);

    LOG_INFO("Step: Read matrix (COO)");
	COOMatrix m;
    readMatrixCOO("datasets/prova", &m);
	printCOO(&m);
	COOMatrix* arr = malloc(sizeof(CSRMatrix)*4);
    LOG_INFO("Step completed: Read matrix (COO)");
	splitCOOMatrix(&m, arr, 4);

	int i;
	for (i=0; i<4; i++){
		printf("======COOMatrix %d=======\n", i);
		printCOO(&arr[i]);
	}
	CSRMatrix* res = malloc(sizeof(CSRMatrix)*4);
	COOListToCSR(arr, res, 4);
	for (i=0; i<4; i++){
		printf("======CSRMatrix %d=======\n", i);
		printCSR(&res[i]);
	}



    LOG_INFO("Step: Convert to CSR");
	CSRMatrix c;
	COOToCSR(&m,&c);
    LOG_INFO("Step completed: Convert to CSR");

	CSRMatrix c1;
	randomInitCOO(&c1, 10, 10, 5, 50);
	printCOO(&c1);

    logger_close();
    LOG_INFO("=== Program end ===");
    return 0;
}*/
