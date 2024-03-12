#include "main_node.h"
#include "mapreduce.h"

ba::awaitable<void> MainNode::startAccept(uint16_t port) {
    const auto executor = co_await ba::this_coro::executor;
    tcp::acceptor acceptor{executor, {tcp::v4(), port}};
    for (;;) {
        auto socket = co_await acceptor.async_accept(ba::use_awaitable);
        std::make_shared<Client>(std::move(socket), *this)->start();
    }
}

ba::awaitable<void> MainNode::dataProcessing(std::shared_ptr<Client> client, std::string &&path) {
    auto data = std::make_shared<SshConnData>();
    data->ip = client->getIp();
    data->user = "dts";
    data->pass = "dts";
    data->path = path;
    auto list = co_await getMr()->dataSplitConn<SshConnection>(
                        co_await CreateConnection::init().connection<SshConnection, SshConnData>(data));
    // list.size();
    // workers.size();

    //co_await (*(workers.begin()))->mapper(*(list.begin()), data);
    for (auto &el : list) {
        std::cout << "-------------------------" << std::endl;
        std::cout << el->begin << " " << el->end << std::endl;
        co_await (*(workers.begin()))->mapper(el, data);
    }
            
}
