//
// Created by rey on 11/13/21.
//

#include "Cache.h"
#include <stdexcept>

void CacheRecord::setFullyCached() {

    pthread_mutex_lock(&mutex);
    this->is_fully_cached = true;
    pthread_mutex_unlock(&mutex);

}

CacheRecord::CacheRecord(Cache* cache) {

    Logger::log("NEW RECORD CREATED/COPIED", true);
    this->cache = cache;
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&condVar, nullptr);
    observersCount = 0;

}

void CacheRecord::write(const std::string &str) {

    pthread_mutex_lock(&mutex);
    try {
        data.append(str);
    }
    catch (std::bad_alloc &a) {
        Logger::log("Proxy ran out of memory. Shutting down...", false);
        cache->setRanOutOfMemory();
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&condVar);

}

std::string CacheRecord::read(size_t start, size_t length) {

    pthread_mutex_lock(&mutex);
    std::string partData;
    while (start >= data.size() && !is_fully_cached && !is_broken) {
        pthread_cond_wait(&condVar, &mutex);
    }
    if (data.size() - start < length) {
        partData = data.substr(start, data.size() - start);
    }
    partData = data.substr(start, length);
    pthread_mutex_unlock(&mutex);
    return partData;

}

bool CacheRecord::isFullyCached() {

    pthread_mutex_lock(&mutex);
    bool cached = is_fully_cached;
    pthread_mutex_unlock(&mutex);
    return cached;

}

void CacheRecord::incrementObserversCount() {

    pthread_mutex_lock(&mutex);
    ++observersCount;
    Logger::log("Incremented! Observer count is " + std::to_string(observersCount) , false);
    pthread_mutex_unlock(&mutex);

}

void CacheRecord::decrementObserversCount() {

    pthread_mutex_lock(&mutex);
    --observersCount;
    Logger::log("Decremented! Observer count is " + std::to_string(observersCount) , false);
    pthread_mutex_unlock(&mutex);

}

int CacheRecord::getObserversCount() {

    pthread_mutex_lock(&mutex);
    int cnt = observersCount;
    pthread_mutex_unlock(&mutex);
    return cnt;

}

void CacheRecord::setBroken() {

    pthread_mutex_lock(&mutex);
    is_broken = true;
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&condVar);

}

bool CacheRecord::isBroken() {

    pthread_mutex_lock(&mutex);
    bool broken = is_broken;
    pthread_mutex_unlock(&mutex);
    return broken;

}

void CacheRecord::wakeUpReaders(){

    pthread_cond_broadcast(&condVar);

}

CacheRecord::~CacheRecord() {

    std::string().swap(data);
    pthread_mutex_destroy(&mutex);

}