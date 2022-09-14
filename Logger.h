#pragma once

#include <string>
#include <iostream>
#include "noncopyable.h"

enum class LogLevel : std::uint8_t {
    kInfo,
    kError,
    kFatal,
    kDebug
};

class Logger : noncopyable {
public:
    static Logger& Instance();
    void set_log_level(LogLevel level);
    void Log(std::string msg);
private:
    LogLevel log_level_;
};

#define BUF_LEN 1024


#define LOG_INFO(log_msg_format, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::kInfo); \
        char buf[BUF_LEN] = {0}; \
        snprintf(buf, BUF_LEN, log_msg_format, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0) 

#define LOG_ERROR(log_msg_format, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::kError); \
        char buf[BUF_LEN] = {0}; \
        snprintf(buf, BUF_LEN, log_msg_format, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0) 

#define LOG_FATAL(log_msg_format, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::kFatal); \
        char buf[BUF_LEN] = {0}; \
        snprintf(buf, BUF_LEN, log_msg_format, ##__VA_ARGS__); \
        logger.Log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG
#define LOG_DEBUG(log_msg_format, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::kDebug); \
        char buf[BUF_LEN] = {0}; \
        snprintf(buf, BUF_LEN, log_msg_format, ##__VA_ARGS__); \
        logger.Log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(log_msg_format, ...)
#endif

