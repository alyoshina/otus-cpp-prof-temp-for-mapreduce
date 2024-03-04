#include "client.h"
#include "server.h"
#include "mapreduce.h"

ba::awaitable<void> Client::read() {
    //host
    std::cout << socket.remote_endpoint().address().to_string() << std::endl;
    std::string ip = socket.remote_endpoint().address().to_string();

    //server.addWorker(shared_from_this());
    try {
        co_await ba::async_read_until(socket, buffer, "\n", ba::use_awaitable);
        std::string readData(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
        if (readData == std::string("worker")) {
            server.addWorker(shared_from_this());
        } else {
            //path
            std::string path = readData;
            boost::trim(path);
            auto sshConn = co_await CreateConnection::init().connection<SshConnection, Client>(shared_from_this(), std::move(path));
            auto list = co_await server.getMr()->dataSplitConn<SshConnection>(sshConn);
            // std::cout << "sshConn->getSize()=" << sshConn->getSize() << std::endl;
            // //sshConn->read();
            // std::string line;
            // while (sshConn->getline(line)) {
            //     std::cout << "line=" << line << std::endl;
            // }

        }
    } catch (const boost::system::system_error& ex) {
        if (ex.code() != ba::error::eof) {
            throw;
        }
        co_return;
    }
    for (;;) {
        buffer.consume(buffer.size());
        try {
            co_await ba::async_read_until(socket, buffer, "\n", ba::use_awaitable);
            std::cout << "read socket" << std::endl;
            std::string readData = std::string(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            std::string path(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            std::cout << "path=" << path << std::endl;
           
        } catch (const boost::system::system_error& ex) {
            if (ex.code() != ba::error::eof) {
                throw;
            }
            //server.leaveToWorker(shared_from_this());

            break;
        }
    }
    std::cout << "end socket" << std::endl;
}