#pragma once

#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t micro_seconds_since_epoch);
    static Timestamp now();
    std::string ToString() const;

private:
    time_t micro_seconds_since_epoch_;
};