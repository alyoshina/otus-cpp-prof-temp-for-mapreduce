#pragma once

#include "thread_pool.h"
#include "node.h"

#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/steady_timer.hpp> 

/**
*  @brief  WorkerNode is client for the MainNode. 
* It executes the @map and @reduce functions for the section received from the MainNode.
* 
*/
class WorkerNode : public ThreadPool, public Node, public std::enable_shared_from_this<WorkerNode> {
public:
    WorkerNode(std::shared_ptr<MapReduce> mr)
    : ThreadPool(2)
    , Node(mr)
    , socket(threadPool)
    , timer(threadPool)
    {
    }
    ~WorkerNode() {
        std::cout << "delete WorkerNode" << std::endl;
    }
    
    /**
    *  @brief  Connecting WorkerNode to the server.
    *  @param[in]  ip is server address
    *  @param[in]  port is server port
    * 
    */
    void connect(std::string ipAddress, uint16_t p) {
        ip = ipAddress;
        port = p;
        ba::co_spawn(threadPool, [self = shared_from_this()] () { return self->connect(); }, ba::detached);

    }
    /** @brief  An asynchronous connect a socket.
    */
    ba::awaitable<void> connect() {
        for (;;) {           
            if (!socket.is_open()) {
                boost::system::error_code ec;
                socket.connect(tcp::endpoint(ba::ip::address::from_string(ip), port), ec);
                if (!ec) {
                    ba::co_spawn(threadPool, [self = shared_from_this()] () { return self->send(); }, ba::detached);
                } else {
                    socket.close();
                }
            }
            timer.expires_from_now(std::chrono::seconds(5));
            co_await timer.async_wait(ba::use_awaitable);
        }
    }
    void disconnect() {
    }
    void stop() {
        socket.close();
        timer.cancel();
    }
    ba::awaitable<void> send();
private:
    //std::weak_ptr<MapReduce> mr;
    tcp::socket socket;
    ba::steady_timer timer;
    ba::streambuf buffer;
    std::string ip;
    uint16_t port;

    
};