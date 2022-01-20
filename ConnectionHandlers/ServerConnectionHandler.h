//
// Created by rey on 11/10/21.
//

#ifndef CACHEPROXY_SERVERCONNECTIONHANDLER_H
#define CACHEPROXY_SERVERCONNECTIONHANDLER_H

#include <iostream>
#include <netdb.h>
#include <codecvt>
#include <locale>
#include <netinet/tcp.h>
#include <bitset>
#include <poll.h>
#include <unistd.h>
#include "../MyCache/Cache.h"

class ServerConnectionHandler {
private:
    int serverSocket;
    Logger *logger;
    CacheRecord *cacheRecord;

public:

    ServerConnectionHandler(int socket, CacheRecord *record);

    int getSocket() const { return serverSocket; };

    CacheRecord *getCacheRecord() { return cacheRecord; };

    void setRecordInvalid() { cacheRecord->setBroken(); };

    ~ServerConnectionHandler();

    void receive();

    bool sendRequest(const std::string &request) const;
};


#endif //CACHEPROXY_SERVERCONNECTIONHANDLER_H
