//
// Created by rey on 11/7/21.
//
#include "CacheProxy.h"

std::vector<std::tuple<int, Cache *> *> clientArgs; // args for client thread routine
std::vector<std::tuple<int, CacheRecord *, const std::string> *> serverArgs; // args for server thread routine

pthread_mutex_t CacheProxy::mapMutex;
std::map<pthread_t, bool> CacheProxy::handlerStatus; // statuses of handlers to join them later

CacheProxy::CacheProxy(int port) {

    Logger::log("CacheProxy :: Starting proxy on port " + std::to_string(port), false);

    this->proxySocket = createProxySocket(port);
    if (proxySocket == -1) {
        Logger::log("CacheProxy :: Socket creation failed. Shutting down.", false);
        return;
    }

    if (!initProxySocket()) {
        Logger::log("CacheProxy :: Shutting down due to an internal error.", false);
        throw std::runtime_error("failed to bind socket on port " + std::to_string(port));
    }

    this->proxyOn = true;
    this->cache = new Cache();

    if (cache == nullptr) {
        Logger::log("Failed to create cache", false);
        stopProxy();
    }
    memset(&address, 0, sizeof(address));

    pthread_mutex_init(&stoppingMutex, nullptr);
    pthread_mutex_init(&mapMutex, nullptr);

}

int CacheProxy::createProxySocket(int port) {

    this->address.sin_port = htons(port);
    this->address.sin_family = AF_INET;
    this->address.sin_addr.s_addr = htonl(INADDR_ANY);
    return socket(AF_INET, SOCK_STREAM, 0);

}

bool CacheProxy::initProxySocket() {

    if (bind(proxySocket, (sockaddr *) &address, sizeof(address)) == -1) {
        Logger::log("CacheProxy :: Socket bind failed. Shutting down.", false);
        return false;
    }

    if (listen(proxySocket, 510) == -1) {
        Logger::log("CacheProxy :: listen(2) failed. Shutting down.", false);
        return false;
    }

    return true;
}

int connectToServer(const std::string &host) {

    int serverSocket;
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Logger::log("Failed to open new socket", false);
        return -1;
    }

    struct hostent *hostinfo = gethostbyname(host.data());
    if (hostinfo == nullptr) {
        Logger::log("Unknown host" + host, false);
        close(serverSocket);
        return -1;
    }

    struct sockaddr_in sockaddrIn{};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(80);
    sockaddrIn.sin_addr = *((struct in_addr *) hostinfo->h_addr);

    if ((connect(serverSocket, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn))) == -1) {
        Logger::log("Cannot connect to" + host, false);
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

void cleanupServer(void *arg) { // server thread cancellation handler

    Logger::log("Server # " + std::to_string(pthread_self()) + " freeing", true);
    auto *server = (ServerConnectionHandler *)arg;
    close(server->getSocket());
    server->setRecordInvalid();
    server->getCacheRecord()->wakeUpReaders();
    delete server;
    Logger::log("Server # " + std::to_string(pthread_self()) + " freed resources", true);

}

void *serverHandlerRoutine(void *args) {

    auto *arguments = (std::tuple<int, CacheRecord *, const std::string> *) args;
    auto *server = new ServerConnectionHandler(std::get<0>(*arguments), std::get<1>(*arguments));

    pthread_cleanup_push(cleanupServer, server);

        const auto &request = std::get<2>(*arguments);
        if (!server->sendRequest(request)) {
            pthread_exit(nullptr);
        }
        Logger::log("Sent request to server", true);
        server->receive();

    pthread_cleanup_pop(0);

    close(server->getSocket());
    CacheProxy::setHandlerDone(pthread_self());
    delete server;
    Logger::log("Server #" + std::to_string(pthread_self()) + " done and freed resources", true);

    return reinterpret_cast<void *>(0);
}

bool createServer(CacheRecord *record, const std::string &host, const std::string &request) {
    // create server thread if page is not cache

    Logger::log("createServer :: Record observer count " + std::to_string(record->getObserversCount()), true);

    int serverSocket = connectToServer(host);
    if (serverSocket == -1) {
        Logger::log("Cannot connect to: " + host + " closing connection.", false);
        return false;
    }
    Logger::log("Connected to server " + host, true);

    auto args = new std::tuple<int, CacheRecord *, const std::string>(serverSocket, record, request);
    serverArgs.push_back(args);

    pthread_t serverThread;
    if (pthread_create(&serverThread, nullptr, serverHandlerRoutine, (void *) args) != 0) {
        Logger::log("CacheProxy :: Failed to create server thread", false);
        return false;
    }

    CacheProxy::addHandler(serverThread);
    Logger::log("Created server thread", true);

    return true;
}

void cleanupClient(void *arg) { // сlient's thread cancellation handler

    Logger::log("Client # " + std::to_string(pthread_self()) + " freeing", true);
    auto *client = (ClientConnectionHandler *) arg;
    close(client->getSocket());
    delete client;
    Logger::log("Client # " + std::to_string(pthread_self()) + " freed resources", true);

}

void *clientHandlerRoutine(void *args) {

    auto *arguments = (std::tuple<int, Cache *> *) args;
    auto socket = std::get<0>(*arguments);
    auto *client = new ClientConnectionHandler(socket, std::get<1>(*arguments));
    pthread_cleanup_push(cleanupClient, client);

        if (client->receive()) {

            auto url = client->getURL();
            auto *cache = client->getCache();

            if (!cache->isCached(url)) {
                client->setCacheRecord(cache->addRecord(url));
                Logger::log(url + " is not cached yet.", true);
                if (!createServer(client->getCacheRecord(), client->getHost(), client->getRequest())) {
                    Logger::log("Failed to create new server", false);
                    pthread_exit((void *) -1);
                }
            } else {
                Logger::log(url + " is cached.", true);
                client->setCacheRecord(cache->getRecord(url));
            }
            client->getCacheRecord()->incrementObserversCount();
            client->handle();
            client->getCacheRecord()->decrementObserversCount();

        } else {
            Logger::log("CacheProxy :: Error while parsing client's request", false);
        }
        Logger::log("Client #" + std::to_string(socket) + " done reading their request.", true);

    pthread_cleanup_pop(0);

    close(client->getSocket());
    CacheProxy::setHandlerDone(pthread_self());
    delete client;
    Logger::log("Client #" + std::to_string(pthread_self()) + " done and freed resources", true);
    return reinterpret_cast<void *>(0);
}

void *signalHandler(void *args) { // thread that sits and waits for incoming signals to notify proxy about

    auto *proxy = (CacheProxy *) args;
    sigset_t mask;
    int err, signo;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    err = sigwait(&mask, &signo);
    Logger::log("CacheProxy :: in signalHandler:: signo = " + std::to_string(signo), false);

    if (err != 0) {
        Logger::log("CacheProxy :: signalHandler:: error in sigwait()", false);
        pthread_exit(nullptr);
    }

    if (signo == SIGINT) {
        Logger::log("CacheProxy :: stopping Proxy and closing socket", false);
        proxy->stopProxy();
        close(proxy->getSocket());
    }

    return reinterpret_cast<void *>(0);
}

void CacheProxy::run() {

    /* Block SIGINT so neither proxy nor it's children threads do not receive it*/
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        Logger::log("CacheProxy :: signalHandler:: error in pthread_sigmask()", false);
        pthread_exit(nullptr);
    }

    /* Create separate thread for SIGINT handling */
    pthread_t signalThread;
    if (pthread_create(&signalThread, nullptr, signalHandler, (void *) this) != 0) {
        Logger::log("CacheProxy :: Failed to create server thread", false);
        return;
    }

    /* Detach it so we don't need to wait to join it when exiting */
    if (pthread_detach(signalThread) != 0) {
        Logger::log("CacheProxy :: Failed to detach single handling thread", false);
    }

    /* Create poll file descriptors set and include proxy socket in it */
    int selectedDescNum;
    int timeOut = 100000;

    std::vector<pollfd> pollingSet;
    pollfd fd{};
    fd.fd = proxySocket;
    fd.events = POLLIN;
    fd.revents = 0;
    pollingSet.push_back(fd);

    /* While everything is OK poll proxy socket, accept new clients and create new threads for them */
    while (!shouldStop() && !cache->ranOutOfMemory()) {
        if ((selectedDescNum = poll(pollingSet.data(), pollingSet.size(), timeOut)) == -1) {
            Logger::log("CacheProxy :: run poll(2) internal error. Shutting down", false);
            stopProxy();
        }

        /* If we catch SIGINT and closed proxy socket poll returns and we get here. We should stop in this case */
        if (shouldStop()) {
            break;
        }

        if (selectedDescNum > 0) {
            if (pollingSet[0].revents == POLLIN) {
                pollingSet[0].revents = 0;

                int newClientFD;

                if ((newClientFD = accept(proxySocket, nullptr, nullptr)) == -1) {
                    Logger::log("CacheProxy :: Failed to accept new connection", false);
                    continue;
                }
                Logger::log("CacheProxy :: Accepted new connection from client #" + std::to_string(newClientFD), true);

                pthread_t newThread;

                auto args = new std::tuple<int, Cache *>(newClientFD, cache);
                clientArgs.push_back(args);

                if (pthread_create(&newThread, nullptr, clientHandlerRoutine, (void *) args) != 0) {
                    Logger::log("CacheProxy :: Failed to create new thread", false);
                    stopProxy();
                }

                addHandler(newThread);
            }
        }
        /* Join threads thar are done */
        cleanDoneHandlers();
    }

    /* when the main loop breaks cancel all threads */
    cancelHandlers();

    /* wake up threads that are sleeping on a cond var*/
    if (!cache->empty()) {
        cache->wakeUpReaders();
    }

    /* and join them all */
    cleanHandlers();
}

template<typename T>
void clearVector(std::vector<T> &vec) {

    std::vector<T>().swap(vec);
    vec.clear();

}

void CacheProxy::cancelHandlers() {

    pthread_mutex_lock(&mapMutex);
    for (auto handler : handlerStatus) {
        pthread_cancel(handler.first);
        Logger::log("CacheProxy :: cancelled thread #" + std::to_string(handler.first), true);
    }
    pthread_mutex_unlock(&mapMutex);

}

void CacheProxy::addHandler(pthread_t tid) {

    pthread_mutex_lock(&mapMutex);
    handlerStatus.insert(std::make_pair(tid, false));
    pthread_mutex_unlock(&mapMutex);

}

void CacheProxy::setHandlerDone(pthread_t tid) {

    pthread_mutex_lock(&mapMutex);
    auto itr = handlerStatus.find(tid);
    if (itr != handlerStatus.end()) {
        (*itr).second = true;
    }
    pthread_mutex_unlock(&mapMutex);

}

void CacheProxy::removeHandler(pthread_t tid) {

    pthread_mutex_lock(&mapMutex);
    auto itr = handlerStatus.find(tid);
    if (itr != handlerStatus.end()) {
        handlerStatus.erase(itr);
    }
    pthread_mutex_unlock(&mapMutex);

}

void CacheProxy::cleanDoneHandlers() {

    pthread_mutex_lock(&mapMutex);
    for (auto it = handlerStatus.begin(); it != handlerStatus.end();) {
        if ((*it).second) {
            void *tret;
            pthread_t tid = (*it).first;
            pthread_join(tid, &tret);
            Logger::log(
                    "CacheProxy :: joined thread #" + std::to_string(tid) + " status = " + std::to_string((long) tret),
                    true);
            it = handlerStatus.erase(it);
        } else {
            ++it;
        }
    }
    pthread_mutex_unlock(&mapMutex);

}


void CacheProxy::cleanHandlers() {

    while (!handlerStatus.empty()) {
        for (auto it = handlerStatus.cbegin(), next_it = it; it != handlerStatus.cend(); it = next_it) {
            ++next_it;
            void *tret;
            pthread_t tid = (*it).first;
            Logger::log("CacheProxy :: joining thread #" + std::to_string(tid), true);
            pthread_join(tid, &tret);
            Logger::log(
                    "CacheProxy :: joined thread #" + std::to_string(tid) + " status = " + std::to_string((long) tret),
                    true);
            removeHandler(tid);
        }
    }

}

void CacheProxy::stopProxy() {

    Logger::log("Stopping proxy", true);
    pthread_mutex_lock(&stoppingMutex);
    this->proxyOn = false;
    pthread_mutex_unlock(&stoppingMutex);

}

bool CacheProxy::shouldStop() {

    pthread_mutex_lock(&stoppingMutex);
    bool stop = this->proxyOn;
    pthread_mutex_unlock(&stoppingMutex);
    return !stop;

}

void CacheProxy::clearArgs() {

    for (auto arg : clientArgs) {
        delete arg;
    }
    clearVector(clientArgs);
    for (auto arg : serverArgs) {
        delete arg;
    }
    clearVector(serverArgs);

}

CacheProxy::~CacheProxy() {

    delete cache;
    clearArgs();
    pthread_mutex_destroy(&stoppingMutex);
    pthread_mutex_destroy(&mapMutex);
    close(proxySocket);

}