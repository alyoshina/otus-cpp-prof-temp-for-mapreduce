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

class MainNode;
class Section;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(tcp::socket s, MainNode& mn)
        : socket(std::move(s))
        , mainNode(mn) {
    }
    ~Client() // = default;
    {
        std::cout << "delete Client" << std::endl;
    }
    void start() {
        ba::co_spawn(socket.get_executor()
                    , [self = shared_from_this()] { return self->read(); }, ba::detached);
    }
    std::string getIp() {
        return socket.remote_endpoint().address().to_string();
    }
    void stop();
    ba::awaitable<void> mapper(std::shared_ptr<Section> s, std::shared_ptr<SshConnData> d);

private:
    tcp::socket socket;
    MainNode& mainNode;
    ba::streambuf buffer;
    std::string user;
    std::string inDataPath;

    ba::awaitable<void> read();
};