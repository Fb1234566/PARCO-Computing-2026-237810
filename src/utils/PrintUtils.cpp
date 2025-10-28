//
// Created by universita on 10/10/25.
//

#include "PrintUtils.h"

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace utils {
    void PrintUtils::printColored(const string &strToPrint, const TerminalColor &color) {
        cout << getAnsiCode(color) << strToPrint << getAnsiCode(TerminalColor::DEFAULT) << endl;
    }

    const char *PrintUtils::getAnsiCode(const TerminalColor color) {
        switch (color) {
            case TerminalColor::BLACK: return "\033[30m";
            case TerminalColor::RED: return "\033[31m";
            case TerminalColor::GREEN: return "\033[32m";
            case TerminalColor::YELLOW: return "\033[33m";
            case TerminalColor::BLUE: return "\033[34m";
            case TerminalColor::MAGENTA: return "\033[35m";
            case TerminalColor::CYAN: return "\033[36m";
            case TerminalColor::WHITE: return "\033[37m";
            case TerminalColor::BRIGHT_BLACK: return "\033[90m";
            case TerminalColor::BRIGHT_RED: return "\033[91m";
            case TerminalColor::BRIGHT_GREEN: return "\033[92m";
            case TerminalColor::BRIGHT_YELLOW: return "\033[93m";
            case TerminalColor::BRIGHT_BLUE: return "\033[94m";
            case TerminalColor::BRIGHT_MAGENTA: return "\033[95m";
            case TerminalColor::BRIGHT_CYAN: return "\033[96m";
            case TerminalColor::BRIGHT_WHITE: return "\033[97m";
            case TerminalColor::DEFAULT:
            default: return "\033[0m";
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


    void PrintUtils::printProgressNewLine(const int current, const int maxElements, std::ostream &out) {
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


    static std::mutex g_log_mutex;

    void PrintUtils::logToFile(const std::string &message, const std::string &filepath, bool append) {
        std::lock_guard<std::mutex> lock(g_log_mutex);

        std::ofstream ofs;
        ofs.open(filepath, append ? std::ios::out | std::ios::app : std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            std::cerr << "PrintUtils::logToFile - impossibile aprire file di log: " << filepath << std::endl;
            return;
        }

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        ofs << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " - " << message << std::endl;
    }
} // Utils
