#pragma once

#include <iostream>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    void log(const std::string& message) {
        if (mute) {
            return;
        } 
        std::cout << message << std::endl;
    };
    void setMute(bool m) {
        mute = m;
    };
private:
    Logger() = default;
    Logger(const Logger&) = delete;
    ~Logger() = default;

    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    bool mute {false};
};