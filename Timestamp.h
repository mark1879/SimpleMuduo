#pragma once

#include <iostream>
#include <string>

class TimeStamp {
public:
    TimeStamp();
    explicit TimeStamp(int64_t micro_seconds_since_epoch);
    static TimeStamp now();
    std::string ToString() const;

private:
    time_t micro_seconds_since_epoch_;
};