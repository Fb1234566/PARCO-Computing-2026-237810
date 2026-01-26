#ifndef DELIVERABLE1_2025_2026_EXPORTER_H
#define DELIVERABLE1_2025_2026_EXPORTER_H
#include <cstddef>

typedef struct {
    char** s;
    int* offsets;
    int count;
} Header;

typedef struct {
    double* value;
    int len;
} Values;

void appendToCSV(Header* h = NULL, Values* v = NULL, char* path = NULL);



#endif //DELIVERABLE1_2025_2026_EXPORTER_H