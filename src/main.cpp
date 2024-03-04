#include "server.h"
#include "mapreduce.h"

#include <boost/asio/thread_pool.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main([[maybe_unused]]int argc, [[maybe_unused]]char** argv) {

    try {
        {
            mapper_t mapper = [] (mapper_element_t& s) { return s; };
            reducer_t reducer = [] (reducer_element_t res, reducer_element_t &el) {
                auto min = std::min(res.first.size(), el.first.size());
                el.second = 0;
                for (std::size_t i = 0; i < min; i++) {
                    el.second = i + 1;
                    if (res.first[i] != el.first[i]) {
                        break;
                    }
                }
                if (res.first.size() && el.second > res.second) {
                    res.second = el.second;
                }
                if (el.second == res.first.size() && el.second < el.first.size()) {
                    el.second += 1;
                }
                return res;
            };

            ba::io_context ioContext;
            ba::signal_set signals{ioContext, SIGINT, SIGTERM};
            signals.async_wait([&](auto, auto) { std::cout << "ioContext stop" << std::endl;  ioContext.stop(); });

            auto mr = std::make_shared<MapReduce>(mapper, reducer);
            //worker
            mr->run();

            ioContext.run();
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}