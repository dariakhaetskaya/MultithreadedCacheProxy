//
// Created by rey on 11/10/21.
//

#include "ServerConnectionHandler.h"

ServerConnectionHandler::ServerConnectionHandler(int socket, CacheRecord *record) : logger((new Logger())) {
    this->serverSocket = socket;
    this->cacheRecord = record;
}

void ServerConnectionHandler::receive() {
    char buffer[BUFSIZ];
    ssize_t len = 1;

    while (len > 0) {
        pthread_testcancel();
        if (cacheRecord->getObserversCount() == 0) {
            cacheRecord->setBroken();
            Logger::log("No one is currently reading url from server #" + std::to_string(serverSocket) + " closing connection.", true);
            return;
        }
        len = read(serverSocket, buffer, BUFSIZ);

        if (len > 0) {
            cacheRecord->write(std::string(buffer, len));
            continue;
        }

        if (len < 0) {
            Logger::log("Failed to read data", true);
            cacheRecord->setBroken();
            return;
        }

        if (len == 0) {
            Logger::log("Client #" + std::to_string(serverSocket) + " done writing. Closing connection", true);
            cacheRecord->setFullyCached();
            return;
        }
    }
}

ServerConnectionHandler::~ServerConnectionHandler() {
    delete logger;
    close(serverSocket);
}

bool ServerConnectionHandler::sendRequest(const std::string& request) const {
    ssize_t len = write(serverSocket, request.data(), request.size());

    if (len == -1) {
        Logger::log("Failed to send data to server", false);
        return false;
    }
    return true;
}

