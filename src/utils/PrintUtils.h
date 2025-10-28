//
// Created by universita on 10/10/25.
//

#ifndef DELIVERABLE1_2025_2026_PRINTUTILS_H
#define DELIVERABLE1_2025_2026_PRINTUTILS_H
#include <iostream>
#include <string>
using namespace std;
namespace utils {
    class PrintUtils {
    public:
        enum class TerminalColor {
            DEFAULT,
            BLACK,
            RED,
            GREEN,
            YELLOW,
            BLUE,
            MAGENTA,
            CYAN,
            WHITE,
            BRIGHT_BLACK,
            BRIGHT_RED,
            BRIGHT_GREEN,
            BRIGHT_YELLOW,
            BRIGHT_BLUE,
            BRIGHT_MAGENTA,
            BRIGHT_CYAN,
            BRIGHT_WHITE
        };

        static const char* getAnsiCode(TerminalColor color);
        static void printColored(const string& strToPrint, const TerminalColor& color);
        static void printProgress(int current, int maxElements);
        static void printProgressNewLine(int current, int maxElements, std::ostream& out = std::cout);

    };
} // Utils

#endif //DELIVERABLE1_2025_2026_PRINTUTILS_H