#pragma once

#include <string>
#include <iostream>

//#include <android/log.h>

struct Debug {
    explicit Debug(std::string tag) noexcept : tag{std::move(tag)} {}

    void verbose(const std::string& s) {
//        write(ANDROID_LOG_VERBOSE, s);
    }

    void debug(const std::string& s) {
//        write(ANDROID_LOG_DEBUG, s);
    }

    void info(const std::string& s) {
//        write(ANDROID_LOG_INFO, s);
    }

    void warn(const std::string& s) {
//        write(ANDROID_LOG_WARN, s);
    }

    void error(const std::string& s) {
        std::cerr << s << std::endl;
//        write(ANDROID_LOG_ERROR, s);
    }

    void fatal(const std::string& s) {
//        write(ANDROID_LOG_FATAL, s);
    }

private:
    std::string tag;

//    void write(android_LogPriority prio, const std::string& s) {
//        __android_log_write(prio, tag.c_str(), s.c_str());
//    }
};
