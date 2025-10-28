//
// Created by universita on 10/10/25.
//

#include "PrintUtils.h"

#include <iostream>

namespace utils {
    void PrintUtils::printColored(const string& strToPrint, const TerminalColor& color) {
        cout << getAnsiCode(color) << strToPrint << getAnsiCode(TerminalColor::DEFAULT) << endl;
    }

    const char* PrintUtils::getAnsiCode(const TerminalColor color) {
        switch (color) {
            case TerminalColor::BLACK:         return "\033[30m";
            case TerminalColor::RED:           return "\033[31m";
            case TerminalColor::GREEN:         return "\033[32m";
            case TerminalColor::YELLOW:        return "\033[33m";
            case TerminalColor::BLUE:          return "\033[34m";
            case TerminalColor::MAGENTA:       return "\033[35m";
            case TerminalColor::CYAN:          return "\033[36m";
            case TerminalColor::WHITE:         return "\033[37m";
            case TerminalColor::BRIGHT_BLACK:  return "\033[90m";
            case TerminalColor::BRIGHT_RED:    return "\033[91m";
            case TerminalColor::BRIGHT_GREEN:  return "\033[92m";
            case TerminalColor::BRIGHT_YELLOW: return "\033[93m";
            case TerminalColor::BRIGHT_BLUE:   return "\033[94m";
            case TerminalColor::BRIGHT_MAGENTA:return "\033[95m";
            case TerminalColor::BRIGHT_CYAN:   return "\033[96m";
            case TerminalColor::BRIGHT_WHITE:  return "\033[97m";
            case TerminalColor::DEFAULT:
            default:                                              return "\033[0m";
        }
    }

    void PrintUtils::printProgress(const int current, const int maxElements) {
        if (maxElements <= 0) return;
        const int barWidth = 50;
        float progress = static_cast<float>(current) / maxElements;
        int pos = static_cast<int>(barWidth * progress);

        std::cout << "\r" << getAnsiCode(TerminalColor::CYAN) << "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << "% (" << current << "/" << maxElements << ")"
                  << getAnsiCode(TerminalColor::DEFAULT) << std::flush;

        if (current == maxElements) std::cout << std::endl;
    }


    void PrintUtils::printProgressNewLine(const int current, const int maxElements, std::ostream& out) {
        if (maxElements <= 0) {
            out << "Progress: " << current << "/" << maxElements << "\n";
            out.flush();
            return;
        }

        const int barWidth = 50;
        const float progress = static_cast<float>(current) / maxElements;
        int percent = static_cast<int>(progress * 100.0f);
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;

        static int lastPrintedPercent = -1;

       if ((percent % 10 != 0 && current != maxElements) || percent == lastPrintedPercent) {
            return;
        }

        lastPrintedPercent = percent;

        const int pos = static_cast<int>(barWidth * (static_cast<float>(percent) / 100.0f));

        out << getAnsiCode(TerminalColor::CYAN) << "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) out << "=";
            else if (i == pos) out << ">";
            else out << " ";
        }
        out << "] " << percent << "% (" << current << "/" << maxElements << ")"
            << getAnsiCode(TerminalColor::DEFAULT) << std::endl;
    }

} // Utils