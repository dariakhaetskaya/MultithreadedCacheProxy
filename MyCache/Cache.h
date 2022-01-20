//
// Created by rey on 11/13/21.
//

#ifndef CACHEPROXY_CACHE_H
#define CACHEPROXY_CACHE_H


#include <vector>
#include <map>
#include "../Logger/Logger.h"

class CacheRecord;

class Cache {
private:
    std::map<std::string, CacheRecord *> cache;
    Logger logger;
    bool ran_out_of_memory = false;
    pthread_mutex_t mutex;

public:
    Cache();

    ~Cache();

    bool isCached(const std::string &url);

    CacheRecord *addRecord(const std::string &url);

    void wakeUpReaders();

    bool empty() { return cache.empty(); };

    bool ranOutOfMemory();

    void setRanOutOfMemory();

    CacheRecord *getRecord(const std::string &url);

};


class CacheRecord {
private:
    Cache *cache;
    bool is_fully_cached = false;
    std::string data;
    bool is_broken = false;
    Logger logger;
    pthread_mutex_t mutex;
    pthread_cond_t condVar;
    volatile int observersCount;

public:
    explicit CacheRecord(Cache *cache);

    ~CacheRecord();

    void setBroken();

    bool isBroken();

    std::string read(size_t start, size_t length);

    void write(const std::string &);

    void setFullyCached();

    bool isFullyCached();

    void incrementObserversCount();

    void decrementObserversCount();

    int getObserversCount();

    void wakeUpReaders();
};


#endif //CACHEPROXY_CACHE_H
