#pragma once

#include <string>

#include "noncopyable.h"

#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.set_log_level(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0) 

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.set_log_level(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.set_log_level(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.Log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.set_log_level(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif


enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};


class Logger : noncopyable
{
public:
    static Logger& Instance();
    void set_log_level(int level);
    void Log(std::string msg);
private:
    int log_level_;
};