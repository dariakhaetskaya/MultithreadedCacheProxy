//
// Created by rey on 11/8/21.
//

#include "Logger.h"

bool Logger::debugMode = false;

void Logger::log(const std::string &msg, bool isDebugMsg) {
    if (!isDebugMsg) {
        std::cerr << ANSI_YELLOW << msg << ANSI_RESET << std::endl;
    } else if (debugMode) {
        std::cerr << ANSI_BLUE << msg << ANSI_RESET << std::endl;
    }
}
