#include "MatrixReader.h"
#include "PrintUtils.h"
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <chrono>
using namespace std;

namespace utils {
    map<string, vector<float> > MatrixReader::operator()(const string &path) const {
        // const vector<vector<float> > matrix = readMatrix(path);
        set<COOEntry> coo = readMatrixCOO(path);
        map<string, vector<float> > csrMatrix = CooMatrixToCSR(coo);
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

    map<string, vector<float> > MatrixReader::CooMatrixToCSR(const set<COOEntry> &coo) {
        PrintUtils::printColored("Starting COO to CSR conversion...", PrintUtils::TerminalColor::CYAN);

        map<string, vector<float> > csr;
        if (coo.empty()) return csr;

        int maxRow = 0;
        for (const auto &e: coo) {
            if (e.row > maxRow) maxRow = e.row;
        }

        csr["vals"] = vector<float>();
        csr["elemIndex"] = vector<float>();
        csr["rowPointer"] = vector<float>(maxRow + 1, 0.0f);

        vector<int> rowCounts(maxRow, 0);
        for (const auto &e: coo) {
            rowCounts[e.row - 1]++;
        }

        for (int i = 1; i <= maxRow; ++i) {
            csr["rowPointer"][i] = csr["rowPointer"][i - 1] + static_cast<float>(rowCounts[i - 1]);
        }

        vector<int> insertPos = rowCounts;
        for (int i = 0; i < maxRow; ++i) {
            insertPos[i] = static_cast<int>(csr["rowPointer"][i]);
        }
        int nnz = coo.size();
        csr["vals"].resize(nnz);
        csr["elemIndex"].resize(nnz);

        for (const auto &e: coo) {
            const int row = e.row - 1;
            const int pos = insertPos[row]++;
            if (row < 0 || row >= maxRow) {
                csr["vals"][pos] = e.val;
                csr["elemIndex"][pos] = static_cast<float>(e.col - 1);
            }
        }
        PrintUtils::printColored("COO to CSR conversion completed.", PrintUtils::TerminalColor::GREEN);

        return csr;
    }
} // utils
