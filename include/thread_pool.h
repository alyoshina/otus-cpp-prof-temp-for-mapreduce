#pragma once

#include "utils.h"

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/signal_set.hpp>

/**
*  @brief  ThreadPool is an execution context where functions run on one of a number (@n) of threads.
* Wrapper for ba::thread_pool and ba::signal_set
* 
*/
class ThreadPool {
public:
    ThreadPool(std::size_t n)
    : threadPool(n)
    , signals({threadPool, SIGINT, SIGTERM}) {
        signals.async_wait([this](auto, auto) { std::cout << "threadPool.stop" << std::endl; threadPool.stop(); });
    }
    virtual ~ThreadPool() {
        std::cout << "~ThreadPool" << std::endl;
        threadPool.join();
    }
protected:
    ba::thread_pool threadPool;
    ba::signal_set signals;
};