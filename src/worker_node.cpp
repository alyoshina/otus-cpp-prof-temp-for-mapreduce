
#include "worker_node.h"
#include "mapreduce.h"

ba::awaitable<void> WorkerNode::send() {
    std::cout << "WorkerNode::send" << std::endl;
    try {
        std::string name = "worker\n";
        co_await ba::async_write(socket, ba::buffer(name), ba::use_awaitable);
        std::cout << "async_read_until" << std::endl;

        for (;;) {
            //boost::trim();
            std::cout << "----------------------------" << std::endl;
            ba::streambuf buf;
            co_await ba::async_read_until(socket, buf, "\n", ba::use_awaitable);
            std::cout << "end async_read_until" << std::endl;
            std::istream is(&buf);
            std::string line;
            while (std::getline(is, line)) {
                //std::string readData(ba::buffer_cast<const char*>(buf.data()), buf.size());
                //std::cout << "readData=" << line << std::endl;
                auto section = std::make_shared<Section>();
                auto data = std::make_shared<SshConnData>();
                std::istringstream(line) >> *section >> *data;
                std::cout << "section=" << section->begin << " " << section->end << std::endl;
                std::cout << "data=" << data->ip << " " << data->user << " " << data->pass << " " << data->path << std::endl;

                auto conn = co_await CreateConnection::init().connection<SshConnection, SshConnData>(data);
                auto v = co_await getMr()->mapFhase(conn, section);
                for (std::size_t i = 0; i < v.size(); i++) {
                    std::cout << "v = " << v[i] << std::endl;
                }
            }



            // std::string readData(ba::buffer_cast<const char*>(buf.data()), buf.size());
            // std::cout << "readData=" << readData << std::endl;

            // auto section = std::make_shared<Section>();
            // auto data = std::make_shared<SshConnData>();

            // std::istream(&buf) >> *section >> *data;
            // //std::cout << "section=" << section->begin << " " << section->end << std::endl;
            // //std::cout << "data=" << data->ip << " " << data->user << " " << data->pass << " " << data->path << std::endl;
            // auto conn = co_await CreateConnection::init().connection<SshConnection, SshConnData>(data);

            // auto v = co_await getMr()->mapFhase(conn, section);

            // for (std::size_t i = 0; i < v.size(); i++) {
            //     std::cout << "v = " << v[i] << std::endl;
            // }
        }



        // buffer.consume(buffer.size());
        // for (;;) {
        //     auto record = requestLogRecord();
        //     record += "\n";
        //     co_await asio::async_write(socket, ba::buffer(record), ba::use_awaitable);
        // }
    } catch (std::exception&) {
        stop();
    }
    std::cout << "end WorkerNode::send" << std::endl;
    //timer_.cancel_one();

        //       boost::system::error_code ec;
        //   co_await timer_.async_wait(redirect_error(use_awaitable, ec));
}