#pragma once

#include "client.h"
//#include "mapreduce.h"

#include <iostream>
#include <boost/asio/thread_pool.hpp>
#include <set>

class MapReduce;

class Server {
public:
    Server(std::shared_ptr<MapReduce> mr)
    :
    mr(mr)
    , threadPool(2)
    //ioContext(context)
    //, signals(ioContext, SIGINT, SIGTERM)
    , signals({threadPool, SIGINT, SIGTERM})
    {
        signals.async_wait([this](auto, auto) { std::cout << "threadPool.stop" << std::endl; threadPool.stop(); });
    }
    ~Server() {
        std::cout << "~Server" << std::endl;
        //todo stop
        threadPool.join();
    };
    void listen(uint16_t port) {
        std::cout << "listen" << std::endl;
        ba::co_spawn(threadPool, startAccept(port), ba::detached);
    }
    void addClient(std::shared_ptr<Client> client)
    {
        clients.insert(client);
    }
    void addWorker(std::shared_ptr<Client> client)
    {
        workers.insert(client);
    }
    void deleteClient(std::shared_ptr<Client> client)
    {
        clients.erase(client);
    }
    void deleteWorker(std::shared_ptr<Client> client)
    {
        workers.erase(client);
    }
    std::shared_ptr<MapReduce> getMr() { return mr; }

private:
    //ba::io_context &ioContext;
    std::shared_ptr<MapReduce> mr;
    ba::thread_pool threadPool;
    ba::signal_set signals;
    std::set<std::shared_ptr<Client>> clients;
    std::set<std::shared_ptr<Client>> workers;
    
    ba::awaitable<void> startAccept(uint16_t port);

};