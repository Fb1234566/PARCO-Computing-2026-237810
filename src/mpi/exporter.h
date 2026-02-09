#ifndef DELIVERABLE1_2025_2026_EXPORTER_H
#define DELIVERABLE1_2025_2026_EXPORTER_H

typedef struct {
    char** s;
    int* offsets;
    int count;
} Header;

typedef struct {
    double* value;
    int len;
} Values;

void appendToCSV(Header* h, Values* v, char* path);



#endif //DELIVERABLE1_2025_2026_EXPORTER_H