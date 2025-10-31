#include "DataTable.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace utils {
    bool DataTable::checkDataType(const Cell &data, const DATATYPES expectedDt) {
        switch (expectedDt) {
            case Int: return holds_alternative<int>(data);
            case Bool: return std::holds_alternative<bool>(data);
            case Float: return std::holds_alternative<float>(data);
            case Double: return std::holds_alternative<double>(data);
            case String: return std::holds_alternative<std::string>(data);
            default: return false;
        }
    }

    void DataTable::AddColumn(const string &n, const DATATYPES dt, vector<Cell> data) {
        header.push_back(n);
        columntype.push_back(dt);
        for_each(data.begin(), data.end(), [dt, n](const Cell &d) {
            if (!checkDataType(d, dt)) {
                throw invalid_argument("Type mismatch adding column: " + n);
            }
        });
        values.push_back(std::move(data));
    }

    void DataTable::AddValue(const string &colName, const Cell &data) {
        bool colFound = false;
        int idx = -1;
        while (colFound == false and idx < header.size()) {
            idx++;
            colFound = header[idx] == colName;
        }

        if (!colFound) {
            throw invalid_argument("Column " + colName + "not present");
        }

        if (!checkDataType(data, columntype[idx])) {
            throw invalid_argument("Data type is not compatible with column datatype");
        }
        if (idx >= values.size()) values.resize(header.size());
        values[idx].push_back(data);
    }

    void DataTable::AddValues(string colName, vector<Cell> data) {
        for_each(data.begin(), data.end(), [colName, this](const Cell &d)-> void {
            AddValue(colName, d);
        });
    }

    void DataTable::AddRow(const vector<Cell> &data) {
        if (data.size() > columntype.size()) {
            throw invalid_argument("Too many elements in row");
        }
        for (int i = 0; i < data.size(); i++) {
            if (!checkDataType(data[i], columntype[i])) {
                throw invalid_argument("Data type is not compatible with column datatype");
            }
            AddValue(header[i], data[i]);
        }
    }

    void DataTable::ExportToCSV(const string &filename) const {
        fstream exportFile(filename, ios::out);
        if (!exportFile) throw std::runtime_error("Unable to open file: " + filename);

        //Export Headers
        for (size_t i = 0; i < header.size(); ++i) {
            exportFile << header[i];
            if (i + 1 < header.size()) exportFile << ',';
        }

        exportFile << endl;

        //Export Data
        int maxLenData = 0;
        for_each(values.begin(), values.end(), [&maxLenData](const vector<Cell> d) {
            if (maxLenData < d.size()) {
                maxLenData = d.size();
            }
        });

        for (int row = 0; row < maxLenData; row++) {
            for (int col = 0; col < values.size(); col++) {
                exportFile << values[col][row];
                if (col + 1 < header.size()) exportFile << ',';
            }
            exportFile << endl;
        }
    }

    static std::string escapeCsvField(const std::string &s) {
        if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back('"');
        for (char c: s) {
            if (c == '"') out.append("\"\"");
            else out.push_back(c);
        }
        out.push_back('"');
        return out;
    }

    std::ostream &operator<<(std::ostream &fs, const Cell &c) {
        std::ostream &os = fs;
        std::visit([&os](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                os << escapeCsvField(v);
            } else if constexpr (std::is_same_v<T, bool>) {
                os << (v ? "true" : "false");
            } else {
                os << v;
            }
        }, c);
        return fs;
    }
} // utils
