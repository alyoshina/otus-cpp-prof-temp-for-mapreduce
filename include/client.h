#pragma once

#include "utils.h"

#include "ssh.h"

#include <boost/algorithm/string/trim_all.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <iostream>
#include <memory>
#include <boost/algorithm/string.hpp>

class Server;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(tcp::socket s, Server& server)
        : socket(std::move(s))
        , server(server) {
        //parser = std::make_shared<Parser>(std::make_shared<Lexer>(input));
    }
    ~Client() = default;
    void start() {
        ba::co_spawn(socket.get_executor()
                    , [self = shared_from_this()] { return self->read(); }, ba::detached);
    }
    std::string getIp() {
        return socket.remote_endpoint().address().to_string();
    }
private:
    tcp::socket socket;
    Server& server;
    ba::streambuf buffer;
    //std::stringstream input;
    //std::shared_ptr<Parser> parser;
    std::string user;
    //std::string host;
    std::string inDataPath;

    ba::awaitable<void> read();
};