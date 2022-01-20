cmake_minimum_required(VERSION 3.16)
project(MultithreadedCacheProxy)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_CXX_FLAGS -pthread)

add_executable(CacheProxy main.cpp CacheProxy.cpp ConnectionHandlers/ClientConnectionHandler.cpp HTTP_Parser_NodeJS/http_parser.c Logger/Logger.cpp ConnectionHandlers/ServerConnectionHandler.cpp MyCache/Cache.cpp MyCache/CacheRecord.cpp)