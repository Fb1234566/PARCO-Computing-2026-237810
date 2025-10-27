//
// Created by universita on 06/10/25.
//

#ifndef DELIVERABLE1_2025_2026_MATRIXREADER_H
#define DELIVERABLE1_2025_2026_MATRIXREADER_H

#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std;

namespace utils {
    struct COOEntry {
        int row;
        int col;
        float val;
        bool operator<(const COOEntry& e) const;
    };

    struct CSRMatrix {
        vector<int> rowPointer;
        vector<int> colIndex;
        vector<float> val;
        int rows;
        int cols;
    };

    ostream& operator<<(ostream& os, const CSRMatrix& m);

    class MatrixReader {
    public:
        static vector<int> getMatrixDimensions(const string& filename);
        static int countLines(const string& filename);
        static set<COOEntry> readMatrixCOO(const string& filename);
        CSRMatrix operator()(const string& path) const;
        static CSRMatrix CooMatrixToCSR(const set<COOEntry> &s, const string &filename);
    };
} // utils

#endif //DELIVERABLE1_2025_2026_MATRIXREADER_H