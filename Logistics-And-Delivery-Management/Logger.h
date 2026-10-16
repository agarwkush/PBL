#pragma once
#include <string>

class Logger {
public:
    static void setFile(const std::string& path);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static std::string filePath;
    static void write(const char* level, const std::string& message);
};
