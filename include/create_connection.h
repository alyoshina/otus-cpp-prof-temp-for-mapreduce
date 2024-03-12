#pragma once

class CreateConnection {
public:
    static CreateConnection& init() {
        static CreateConnection instance;
        return instance;
    }
    // template<typename T, typename Client>
    // ba::awaitable<std::shared_ptr<T>> connection(std::shared_ptr<Client> client, std::string &&path) {
    //     auto initiate = [this, client, &path]<typename Handler>(Handler&& handler) mutable {
    //         ba::post(threadPool //client->getParserContext().get_executor()
    //                     , [handler = std::forward<Handler>(handler), this, client, &path]() mutable {
    //             //std::cout << "!!!PATH" << path << std::endl;
    //             //std::string p = client->getIp();
    //             std::string ip = client->getIp();
    //             handler(this->connect<T>(std::move(ip), std::move(path)));
    //         });
    //     };
    //     return ba::async_initiate<decltype(ba::use_awaitable), void(std::shared_ptr<T>)>(initiate, ba::use_awaitable);
    // }

    template<typename T, typename D>
    ba::awaitable<std::shared_ptr<T>> connection(std::shared_ptr<D> data) {
        auto initiate = [this, data]<typename Handler>(Handler&& handler) mutable {
            ba::post(threadPool
                        , [handler = std::forward<Handler>(handler), this, data]() mutable {
                handler(this->connect<T>(data));
            });
        };
        return ba::async_initiate<decltype(ba::use_awaitable), void(std::shared_ptr<T>)>(initiate, ba::use_awaitable);
    }

    ba::thread_pool& getThreadPool() { return threadPool; }
private:
    template<typename T, typename D>
    std::shared_ptr<T> connect(std::shared_ptr<D> data) {
        return std::make_shared<T>(data);
    }
    // template<typename T>
    // std::shared_ptr<T> connect(std::string &&ip, std::string &&path) {
    //     return std::make_shared<T>(ip, path);
    // }

    explicit CreateConnection()
    : threadPool(1)
    , signals({threadPool, SIGINT, SIGTERM})
    {
        signals.async_wait([this](auto, auto) { std::cout << "threadPool.stop" << std::endl; threadPool.stop(); });
    }
    ~CreateConnection() {
        threadPool.join();
    }
    ba::thread_pool threadPool;
    ba::signal_set signals;
};