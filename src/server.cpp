#include "server.h"

/** @brief  Initiates an asynchronous accept operation to wait for a new connection 
*/
ba::awaitable<void> Server::startAccept(uint16_t port) {
    const auto executor = co_await ba::this_coro::executor;
    tcp::acceptor acceptor{executor, {tcp::v4(), port}};
    for (;;) {
        auto socket = co_await acceptor.async_accept(ba::use_awaitable);
        std::make_shared<Client>(std::move(socket), *this)->start();
    }
}