//
// Created by rey on 11/7/21.
//

#include "ClientConnectionHandler.h"

static int getUrlFromHTTP(http_parser *parser, const char *at, size_t len) {

    auto *handler = (ClientConnectionHandler *) parser->data;
    Logger::log("Client sent " + std::string(http_method_str(static_cast<http_method>(parser->method)))
                      + " method", true);

    if (parser->method != 1u && parser->method != 2u) {

        Logger::log("Client sent " + std::string(http_method_str(static_cast<http_method>(parser->method)))
                          + " method. Proxy does not currently support it.", false);
        int sock = handler->getSocket();
        char NOT_ALLOWED[59] = "HTTP/1.0 405 METHOD NOT ALLOWED\r\n\r\n Method Not Allowed";
        write(sock, NOT_ALLOWED, 59);
        return 1;
    }

    handler->setURL(at, len);
    return 0;

}

int handleHeaderField(http_parser *parser, const char *at, size_t len) {

    auto *handler = (ClientConnectionHandler *) parser->data;
    handler->setLastField(at, len);
    return 0;

}

int handleHeaderValue(http_parser *parser, const char *at, size_t len) {

    auto *handler = (ClientConnectionHandler *) parser->data;
    if (handler->getLastField() == "Host") {
        handler->setHost(at, len);
    }

    if (handler->getLastField() == "Connection") {
        handler->setNewHeader(handler->getLastField() + ": " + "close" + "\r\n");
    } else {
        handler->setNewHeader(handler->getLastField() + ": " + std::string(at, len) + "\r\n");

    }
    handler->resetLastField();
    return 0;

}

static int handleHeadersComplete(http_parser *parser) {

    auto *handler = (ClientConnectionHandler *) parser->data;
    handler->setNewHeader("\r\n");
    return 0;

}


ClientConnectionHandler::ClientConnectionHandler(int sock, Cache *cache) : logger((new Logger())) {

    http_parser_settings_init(&settings);
    http_parser_init(&parser, HTTP_REQUEST);this->clientSocket = sock;
    this->parser.data = this;
    this->cache = cache;
    this->settings.on_url = getUrlFromHTTP;
    this->settings.on_header_field = handleHeaderField;
    this->settings.on_header_value = handleHeaderValue;
    this->settings.on_headers_complete = handleHeadersComplete;
    this->url = "";
    this->host = "";
    this->headers = "";
    this->record = nullptr;

}

void ClientConnectionHandler::handle() {

    while(true) {
        pthread_testcancel();
        if (record->isBroken()) {
            return;
        }

        int state;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
        std::string buffer = record->read(readPointer, BUFSIZ);
        pthread_setcancelstate(state, nullptr);
        ssize_t ret = write(clientSocket, buffer.data(), buffer.size());
        if (ret == -1) {
            perror("write failed");
            return;
        } else {
            if (buffer.empty()){
                break;
            }
            readPointer += buffer.size();
        }
    }
}

bool ClientConnectionHandler::receive(){

    char buffer[BUFSIZ];

    std::string request;
    ssize_t len = 1;

    while (len > 0) {
        len = read(clientSocket, buffer, BUFSIZ);

        if (len < 0) {
            Logger::log("Failed to read data from client's socket", true);
            return false;
        }

        if (len == 0) {
            Logger::log("Client #" + std::to_string(clientSocket) + " done writing. Closing connection", true);
            return false;
        }

        request.append(buffer, len);
        memset(buffer, 0, BUFSIZ);
        if (len == BUFSIZ) {
            memset(buffer, 0, BUFSIZ);
        } else {
            len = 0;
        }
    }

    int ret = http_parser_execute(&parser, &settings, request.data(), request.size());

    if (ret != request.size() || parser.http_errno != 0u) {
        Logger::log("Failed to parse http. Errno " + std::to_string(parser.http_errno), false);
        return false;
    }
    return true;

}

std::string ClientConnectionHandler::getURL() {
    return url;
}

void ClientConnectionHandler::setURL(const char *at, size_t len) {
    url.append(at, len);
}

std::string ClientConnectionHandler::getLastField() {
    return lastField;
}

void ClientConnectionHandler::setLastField(const char *at, size_t len) {
    lastField.append(at, len);
}


void ClientConnectionHandler::setNewHeader(const std::string &newHeader) {
    headers.append(newHeader);
}

void ClientConnectionHandler::resetLastField() {
    lastField = "";
}

void ClientConnectionHandler::setHost(const char *at, size_t len) {
    host.append(at, len);
}

ClientConnectionHandler::~ClientConnectionHandler() {
    delete logger;
}

std::string ClientConnectionHandler::getRequest() {
    return "GET " + url + " HTTP/1.0\r\n" + headers;
}


