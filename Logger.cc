
#include "Logger.h"
#include "Timestamp.h"

Logger& Logger::Instance() {
    static Logger logger;
    return logger;
}

void Logger::set_log_level(LogLevel level) {
    log_level_ = level;
}

void Logger::Log(std::string msg) {
    switch (log_level_)
    {
    case LogLevel::kInfo:
        std::cout << "[INFO]";
        break;
     case LogLevel::kError:
        std::cout << "[ERROR]";
        break;
     case LogLevel::kFatal:
        std::cout << "[FATAL]";
        break;
     case LogLevel::kDebug:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    std::cout << TimeStamp::now().ToString() << " : " << msg << std::endl;
}