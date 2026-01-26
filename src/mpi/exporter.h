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

void appendToCSV(Header* h = nullptr, Values* v = nullptr, char* path = nullptr);



#endif //DELIVERABLE1_2025_2026_EXPORTER_H