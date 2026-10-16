#include "Logger.h"
#include <ctime>
#include <fstream>

std::string Logger::filePath;

void Logger::setFile(const std::string& path) {
    filePath = path;
}

void Logger::info(const std::string& message) {
    write("INFO", message);
}

void Logger::warn(const std::string& message) {
    write("WARN", message);
}

void Logger::error(const std::string& message) {
    write("ERROR", message);
}

void Logger::write(const char* level, const std::string& message) {
    if (filePath.empty()) return;
    std::ofstream out(filePath, std::ios::app);
    if (!out.is_open()) return;
    std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    out << stamp << "  " << level << "  " << message << "\n";
}
