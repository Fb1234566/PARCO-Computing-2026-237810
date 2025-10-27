#include "MatrixReader.h"
#include "PrintUtils.h"
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <chrono>
using namespace std;

namespace utils {
    CSRMatrix MatrixReader::operator()(const string &path) const {
        // const vector<vector<float> > matrix = readMatrix(path);
        set<COOEntry> coo = readMatrixCOO(path);
        CSRMatrix csrMatrix = CooMatrixToCSR(coo, path);
        return csrMatrix;
    }

    int MatrixReader::countLines(const string &filename) {
        std::ifstream f(filename);
        std::string curr_line;
        int counter = 0;
        while (std::getline(f, curr_line)) {
            counter++;
        }
        return counter;
    }

    set<COOEntry> MatrixReader::readMatrixCOO(const string &filename) {
        ifstream MatrixFile(filename);

        if (!MatrixFile.is_open()) {
            PrintUtils::printColored("Errore: impossibile aprire il file " + filename, PrintUtils::TerminalColor::RED);
            return {};
        }

        set<COOEntry> COOMatrix;
        PrintUtils::printColored("Reading file", PrintUtils::TerminalColor::CYAN);
        string curr_line;
        string prev_line = "%";
        int rows = 0, cols = 0;
        int row, col;
        float val;
        int numLines = countLines(filename), lineCounter = 0;

        auto t_start = chrono::high_resolution_clock::now();

        while (getline(MatrixFile, curr_line)) {
            lineCounter++;
            if (!curr_line.empty()) {
                if (curr_line[0] != '%') {
                    if (curr_line[0] != '%' && prev_line[0] == '%') {
                        istringstream iss(curr_line);
                        int nnz;
                        iss >> rows >> cols >> nnz;
                        PrintUtils::printColored(
                            "Matrix dimensions: " + to_string(rows) + "x" + to_string(cols) + " (non-zero: " +
                            to_string(nnz) + ")", PrintUtils::TerminalColor::YELLOW);
                    } else {
                        istringstream iss(curr_line);
                        iss >> row >> col >> val;
                        if (row > 0 && col > 0) {
                            COOEntry entry{.row = row, .col = col, .val = val};
                            COOMatrix.insert(entry);
                        }
                        utils::PrintUtils::printProgress(lineCounter, numLines);
                    }
                }
                prev_line = curr_line;
            }
        }

        auto t_end = chrono::high_resolution_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
        PrintUtils::printColored("Parsing completed in " + to_string(ms) + " ms", PrintUtils::TerminalColor::GREEN);
        MatrixFile.close();
        return COOMatrix;
    }

    bool COOEntry::operator<(const COOEntry &e) const {
        if (row < e.row) {
            return true;
        }
        if (row == e.row) {
            return col < e.col;
        }
        return false;
    }

    CSRMatrix MatrixReader::CooMatrixToCSR(const set<COOEntry> &s, const string &filename) {
        PrintUtils::printColored("Starting COO to CSR conversion...", PrintUtils::TerminalColor::CYAN);

        CSRMatrix csr;
        if (s.empty()) return csr;
        const int maxRow = getMatrixDimensions(filename)[0];
        const int maxCols = getMatrixDimensions(filename)[1];

        csr.val = vector<float>();
        csr.colIndex = vector<int>();
        csr.rowPointer = vector<int>(maxRow + 1, 0);
        csr.rows = maxRow;
        csr.cols = maxCols;

        vector<int> rowCounts(maxRow, 0);
        for (const auto &e: s) {
            rowCounts[e.row - 1]++;
        }

        for (int i = 1; i <= maxRow; ++i) {
            csr.rowPointer[i] = csr.rowPointer[i - 1] + rowCounts[i - 1];
        }

        vector<int> insertPos = rowCounts;
        for (int i = 0; i < maxRow; ++i) {
            insertPos[i] = csr.rowPointer[i];
        }
        int nnz = s.size();
        csr.val.resize(nnz);
        csr.colIndex.resize(nnz);

        for (const auto &e: s) {
            const int row = e.row - 1;
            const int pos = insertPos[row]++;
            if (row >= 0 && row <= maxRow) {
                csr.val[pos] = e.val;
                csr.colIndex[pos] = e.col - 1;
            }
        }
        PrintUtils::printColored("COO to CSR conversion completed.", PrintUtils::TerminalColor::GREEN);
        return csr;
    }

    ostream &operator<<(ostream &os, const CSRMatrix &m) {
        os << "Row pointer: [ ";
        for (const auto e: m.rowPointer) {
            os << e << ", ";
        }
        os << "]" << endl << "Column index: [ ";
        for (const auto e: m.colIndex) {
            os << e << ", ";
        }
        os << "]" << endl << "Values: [ ";
        for (const auto e: m.val) {
            os << e << ", ";
        }
        os << "]" << endl;
        return os;
    }
    vector<int> MatrixReader::getMatrixDimensions(const string& filename) {
        ifstream MatrixFile(filename);

        if (!MatrixFile.is_open()) {
            PrintUtils::printColored("Errore: impossibile aprire il file " + filename, PrintUtils::TerminalColor::RED);
            return {};
        }

        set<COOEntry> COOMatrix;
        string curr_line;
        string prev_line = "%";
        int rows = 0, cols = 0;
        vector<int> res = {rows, cols};

        while (getline(MatrixFile, curr_line)) {
            if (!curr_line.empty()) {
                if (curr_line[0] != '%') {
                    if (curr_line[0] != '%' && prev_line[0] == '%') {
                        istringstream iss(curr_line);
                        int nnz;
                        iss >> rows >> cols >> nnz;
                        return vector<int>{rows, cols};
                    }
                }
            }
        }
        return res;
    }
} // utils
