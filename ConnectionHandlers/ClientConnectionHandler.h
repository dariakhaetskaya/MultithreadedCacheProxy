//
// Created by rey on 11/7/21.
//

#ifndef CACHEPROXY_CLIENTCONNECTIONHANDLER_H
#define CACHEPROXY_CLIENTCONNECTIONHANDLER_H

#include "../CacheProxy.h"
#include "../HTTP_Parser_NodeJS/http_parser.h"
#include "ServerConnectionHandler.h"
#include <netdb.h>
#include <netinet/tcp.h>
#include <bitset>

class CacheProxy;

class ClientConnectionHandler {
private:
    int clientSocket;
    Cache *cache;
    std::string lastField;
    http_parser parser;
    http_parser_settings settings;
    std::string url;
    std::string host;
    std::string headers;
    Logger *logger;
    CacheRecord *record;
    size_t readPointer = 0;

public:
    explicit ClientConnectionHandler(int socket, Cache *cache);

    void handle();

    ~ClientConnectionHandler();

    std::string getURL();

    void setURL(const char *at, size_t len);

    void setHost(const char *at, size_t len);

    int getSocket() const { return clientSocket; };

    std::string getLastField();

    void setLastField(const char *at, size_t len);

    void setNewHeader(const std::string &newHeader);

    void resetLastField();

    Cache *getCache() { return cache; };

    void setCacheRecord(CacheRecord *rec) { record = rec; };

    CacheRecord *getCacheRecord() { return record; };

    bool receive();

    std::string getHost() { return host; };

    std::string getRequest();

};


#endif //CACHEPROXY_CLIENTCONNECTIONHANDLER_H
