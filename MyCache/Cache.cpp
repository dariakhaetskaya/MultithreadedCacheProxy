//
// Created by rey on 11/13/21.
//

#include "Cache.h"

Cache::Cache() {

    pthread_mutex_init(&mutex, nullptr);

}

CacheRecord *Cache::getRecord(const std::string &url) {

    if (isCached(url)) {
        pthread_mutex_lock(&mutex);
        auto *rec = cache.at(url);
        pthread_mutex_unlock(&mutex);
        return rec;
    } else {
        return nullptr;
    }

}

bool Cache::isCached(const std::string &url) {

    bool cached, isBroken;
    pthread_mutex_lock(&mutex);
    cached = (cache.find(url) != cache.end());
    if (cached) {
        isBroken = cache.find(url)->second->isBroken();
    }
    pthread_mutex_unlock(&mutex);
    return cached && !isBroken;

}

CacheRecord *Cache::addRecord(const std::string &url) {

    pthread_mutex_lock(&mutex);
    if (cache.find(url) != cache.end()) {
        delete cache.find(url)->second;
    }
    Logger::log("Adding new record for " + url, false);
    auto record = new CacheRecord(this);
    cache.insert(std::make_pair(url, record));
    pthread_mutex_unlock(&mutex);
    return record;

}

bool Cache::ranOutOfMemory() {

    pthread_mutex_lock(&mutex);
    bool ran = ran_out_of_memory;
    pthread_mutex_unlock(&mutex);
    return ran;

}

void Cache::setRanOutOfMemory() {

    pthread_mutex_lock(&mutex);
    ran_out_of_memory = true;
    pthread_mutex_unlock(&mutex);

}

void Cache::wakeUpReaders() {

    pthread_mutex_lock(&mutex);
    for (auto record : cache) {
        record.second->wakeUpReaders();
    }
    pthread_mutex_unlock(&mutex);

}

Cache::~Cache() {

    for (const auto &record : cache) {
        delete record.second;
    }
    pthread_mutex_destroy(&mutex);
    Logger::log("Cache deleted", true);

}