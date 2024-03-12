#include "mapreduce.h"

#include <boost/asio/thread_pool.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main([[maybe_unused]]int argc, [[maybe_unused]]char** argv) {
    //processing сommand line argument
    po::options_description desc {"Options"};
    desc.add_options()
            ("help,h", "MapReduce task launch system")
            ("worker,w", po::value<bool>() -> default_value(true), "for main node value is false, for worker node value is true")
            ("port,p", po::value<unsigned short>() -> default_value(9000), "listening port for main node or main node port for worker node")
            ("ip,i", po::value<std::string>() -> default_value("10.10.10.139"), "main node host for worker node");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }
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
            mr->run(vm["worker"].as<bool>(), vm["port"].as<unsigned short>(), vm["ip"].as<std::string>());

            ioContext.run();
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}