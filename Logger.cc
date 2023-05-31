#include "Logger.h"
#include "Timestamp.h"
#include <iostream>

Logger& Logger::Instance()
{
    static Logger logger;
    return logger;
}

void Logger::set_log_level(int level)
{
    log_level_ = level;
}

void Logger::Log(std::string msg)
{
    switch (log_level_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    std::cout << Timestamp::Now().ToString() << " : " << msg << std::endl;
}