#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void readMatrixCOO(const char* path, COOMatrix* m){
    FILE *f = fopen(path, "r");
    if (!f) {perror("fopen"); return NULL;}

    //parse the comments for the dimensions of the matrix
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
            printf("Found dimensionse: %dx%d %d nnz\n", m->rows, m->cols, m->nnz);
            break;
        } else {
            fprintf(stderr, "Malformed dimension line: %s\n", p);
            break;
        }
    }

    m->row = calloc(m->nnz, sizeof(int));
    m->col = calloc(m->nnz, sizeof(int));
    m->val = calloc(m->nnz, sizeof(double));
	cooList = malloc(m->nnz * sizeof(COOEntry));

	int lineIdx = 0;
    if (!m->row || !m->col || !m->val) {
        perror("calloc");
        free(m->row); free(m->col); free(m->val);
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
            fprintf(stderr, "Malformed line: %s\n", line);
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
}

void COOToCSR(COOMatrix* in, CSRMatrix* out){
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
    	perror("calloc CSR");
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
}

void printCOO(const COOMatrix* m){
    if (!m) return;
    printf("COO Matrix: %dx%d, nnz=%d\n", m->rows, m->cols, m->nnz);
    for (int i = 0; i < m->nnz; ++i) {
        printf("%d %d %g\n", m->row[i] , m->col[i] , m->val[i]);
    }
}

void printCSR(const CSRMatrix* m){
    if (!m) return;
    printf("CSR Matrix: %dx%d, nnz=%d\n", m->rows, m->cols, m->nnz);
    if (!m->rowPtr || !m->col || !m->val) return;

    for (int r = 0; r < m->rows; ++r) {
        int start = m->rowPtr[r];
        int end = (r + 1 <= m->rows) ? m->rowPtr[r + 1] : m->nnz;
        if (start < 0) start = 0;
        if (end > m->nnz) end = m->nnz;
        if (start >= end) continue;

        for (int idx = start; idx < end; ++idx) {
            printf("%d %d %g\n", r, m->col[idx], m->val[idx]);
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
	COOMatrix m;
    readMatrixCOO("datasets/prova", &m);
	printCOO(&m);
	CSRMatrix c;
	COOToCSR(&m,&c);
	printCSR(& c);
    return 0;
}