//
// Created by rey on 11/7/21.
//

#ifndef CACHEPROXY_CACHEPROXY_H
#define CACHEPROXY_CACHEPROXY_H

#include <pthread.h>
#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <iostream>
#include <bitset>
#include <csignal>

#include "ConnectionHandlers/ClientConnectionHandler.h"
#include "MyCache/Cache.h"

class CacheProxy {

private:
    Logger logger;
    sockaddr_in address{};
    int proxySocket;
    bool proxyOn = false;
    Cache *cache;
    pthread_mutex_t stoppingMutex;
    static pthread_mutex_t mapMutex;
    static std::map<pthread_t, bool> handlerStatus;

    static void cleanHandlers();

    int createProxySocket(int port);

    bool initProxySocket();

    static void cleanDoneHandlers();


public:

    CacheProxy(int port);

    ~CacheProxy();

    void run();

    void stopProxy();

    bool shouldStop();

    int getSocket() { return proxySocket; };

    static void addHandler(pthread_t tid);

    static void setHandlerDone(pthread_t tid);

    static void removeHandler(pthread_t tid);

    static void clearArgs();

    static void cancelHandlers();
};


#endif //CACHEPROXY_CACHEPROXY_H
