#ifndef DELIVERABLE1_2025_2026_CSVEXPORTER_H
#define DELIVERABLE1_2025_2026_CSVEXPORTER_H

#include <string>
#include <variant>
#include <vector>

using namespace std;

namespace utils {
    enum DATATYPES {
        Int,
        Bool,
        String,
        Float,
        Double,
    };
    using Cell = variant<int, bool, string, float, double>;
    class DataTable {
    private:
        string name;
        vector<string> header;
        vector<DATATYPES> columntype;
        vector<vector<Cell>> values;
        static bool checkDataType(const Cell &data, DATATYPES expectedDt);
    public:
        void AddColumn(const string &n, DATATYPES dt = Int, vector<Cell> data = vector<Cell>());
        void AddValue(const string &colName, const Cell &data);
        void AddValues(string colName, vector<Cell> data);
        void AddRow(const vector<Cell> &data);
        void ExportToCSV(const string &filename) const;
    };

    ostream& operator<<(ostream& fs, const Cell& c);

} // utils

#endif //DELIVERABLE1_2025_2026_CSVEXPORTER_H