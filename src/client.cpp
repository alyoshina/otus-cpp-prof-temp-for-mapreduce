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
            auto sshConn = co_await CreateConnection::init().connection<SshConnection, Client>(shared_from_this());
            auto list = co_await server.getMr()->dataSplitConn<SshConnection>(sshConn);
            // std::cout << "sshConn->getSize()=" << sshConn->getSize() << std::endl;
            // //sshConn->read();
            // std::string line;
            // while (sshConn->getline(line)) {
            //     std::cout << "line=" << line << std::endl;
            // }

        }
        // std::vector<std::string> data;
        // boost::split(data, readData, boost::is_any_of("@"));
        // if (data.size() > 1) {
        //     user = data[0];

        //     host = data[1];
        //     std::cout << "user=" << user << " host=" << host << std::endl;
        // } else {
        //     std::cout << "Error" << std::endl;
        //     co_return;
        // }
    } catch (const boost::system::system_error& ex) {
        if (ex.code() != ba::error::eof) {
            throw;
        }
        co_return;
    }
    // try {
    //     co_await ba::async_read_until(socket, buffer, "\n", ba::use_awaitable);
    //     std::string readData(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
    //     std::vector<std::string> data;
    //     boost::split(data, readData, boost::is_any_of("@"));
    //     if (data.size() > 1) {
    //         user = data[0];

    //         host = data[1];
    //         std::cout << "user=" << user << " host=" << host << std::endl;
    //     } else {
    //         std::cout << "Error" << std::endl;
    //         co_return;
    //     }
    // } catch (const boost::system::system_error& ex) {
    //     if (ex.code() != ba::error::eof) {
    //         throw;
    //     }
    //     co_return;
    // }
    for (;;) {
        buffer.consume(buffer.size());
        try {
            co_await ba::async_read_until(socket, buffer, "\n", ba::use_awaitable);
            std::cout << "read socket" << std::endl;
            std::string readData = std::string(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            std::string path(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            std::cout << "path=" << path << std::endl;
            ;
            //std::list<std::shared_ptr<Section>>
            //co_await  MapReduce::dataSplitSsh(path.c_str(), std::size_t mNum)



            //input << std::string(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            // if (co_await parser->parse(shared_from_this())) {
            //     std::shared_ptr<ICmd> cmd = parser->getCmd();
            //     std::string response;
            //     if (co_await data.cmdExec(shared_from_this(), cmd)) {
            //         response = std::format("{}OK\n", cmd->getResult());
            //     } else {
            //         response = std::format("ERR {}\n", cmd->getResult());
            //     }
            //     co_await ba::async_write(socket
            //                     , ba::buffer(response.data(), response.size())
            //                     , ba::use_awaitable);
            // }
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