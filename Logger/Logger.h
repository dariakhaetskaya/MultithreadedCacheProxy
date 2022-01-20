//
// Created by rey on 11/7/21.
//

#ifndef CACHEPROXY_LOGGER_H
#define CACHEPROXY_LOGGER_H

#include <iostream>

#define ANSI_RESET "\033[0m"
#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_PURPLE "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"
#define ANSI_PURPLE_BG "\033[45m\033[30m"

class Logger {

public:
    static bool debugMode;

    Logger() = default;

    ~Logger() = default;

    static void log(const std::string &msg, bool isDebugMsg);
};


#endif //CACHEPROXY_LOGGER_H
