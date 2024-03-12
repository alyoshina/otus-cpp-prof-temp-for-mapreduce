#include "client.h"
#include "main_node.h"
#include "mapreduce.h"

void Client::stop() {
    mainNode.deleteClient(shared_from_this());
    mainNode.deleteWorker(shared_from_this());
    socket.close();
}

ba::awaitable<void> Client::mapper(std::shared_ptr<Section> s, std::shared_ptr<SshConnData> d) {
    ba::streambuf buf;
    std::ostream(&buf) << *s << *d << "\n";


    // Section section;
    // std::istream(&buf) >> section;
    // std::cout << "section=" << section.begin << " " << section.end << std::endl;


    co_await ba::async_write(socket, buf, ba::use_awaitable);
    //co_await ba::async_write(socket, buf, ba::use_awaitable);
    // std::string wData(ba::buffer_cast<const char*>(buf.data()), buf.size());
    // std::cout << "wData=" << wData << std::endl;
    // std::cout << "mapper " << s << std::endl;

}

ba::awaitable<void> Client::read() {
    //host
    std::cout << "new Client " << socket.remote_endpoint().address().to_string() << std::endl;

    try {
        co_await ba::async_read_until(socket, buffer, "\n", ba::use_awaitable);
        std::string readData(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
        boost::trim(readData);
        if (readData == std::string("worker")) {
            mainNode.addWorker(shared_from_this());
            //get data from mapper 
        } else {
            std::string path = readData;
            //boost::trim(path);
            // auto sshConn = co_await CreateConnection::init().connection<SshConnection, Client>(shared_from_this(), std::move(path));
            // auto list = co_await mainNode.getMr()->dataSplitConn<SshConnection>(sshConn);
            
            co_await mainNode.dataProcessing(shared_from_this(), std::move(path));

            //
            
            
            // std::cout << "sshConn->getSize()=" << sshConn->getSize() << std::endl;
            // //sshConn->read();
            // std::string line;
            // while (sshConn->getline(line)) {
            //     std::cout << "line=" << line << std::endl;
            // }

        }
    } catch (std::exception&) {
        stop();
        co_return;
    }
    for (;;) {
        buffer.consume(buffer.size());
        try {
            co_await ba::async_read_until(socket, buffer, "\n", ba::use_awaitable);
            // std::cout << "read socket" << std::endl;
            // std::string readData = std::string(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            // std::string path(ba::buffer_cast<const char*>(buffer.data()), buffer.size());
            // std::cout << "path=" << path << std::endl;
           
        } catch (std::exception&) {
            // if (ex.code() != ba::error::eof) {
            //     throw;
            // }
            //server.leaveToWorker(shared_from_this());
            stop();
            break;
        }
    }
    std::cout << "end socket" << std::endl;
}