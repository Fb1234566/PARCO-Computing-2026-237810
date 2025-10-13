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

    class MatrixReader {
    public:
        static int countLines(const string& filename);
        static set<COOEntry> readMatrixCOO(const string& filename);
        map<string, vector<float>> operator()(const string& path) const;
        static map<string, vector<float>> CooMatrixToCSR(const set<COOEntry> &s);
    };
} // utils

#endif //DELIVERABLE1_2025_2026_MATRIXREADER_H