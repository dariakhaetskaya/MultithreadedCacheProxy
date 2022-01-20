#include <iostream>
#include <csignal>
#include "CacheProxy.h"
#include "Logger/Logger.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: main.cpp [port number] [logging mode : debug | info]" << std::endl;
        return -1;
    }

    std::cout << getpid() << std::endl;

    sigset(SIGPIPE, SIG_IGN);
    sigset(SIGSTOP, SIG_IGN);

    struct sigaction act{};
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);

    std::string mode = argv[2];
    Logger::debugMode = !(mode.compare("debug"));

    std::cout << "STARTING PROXY ON PORT " << argv[1] << std::endl;
    std::cout << "Logging mode: " << argv[2] << std::endl;

    try {
        auto cacheProxy = new CacheProxy(atoi(argv[1]));
        cacheProxy->run();
        delete cacheProxy;
        std::cout << "PROXY IS DOWN. Press F to pay respects." << std::endl;
    } catch (std::runtime_error) {
        return -1;
    }

    return 0;
}
