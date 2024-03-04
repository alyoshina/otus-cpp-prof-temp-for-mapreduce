#include "server.h"
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
            ("ip,i", po::value<std::string>() -> default_value("127.0.0.1"), "main node host for worker node")
            ("src,s", po::value<std::string>() -> default_value("./F.txt")
                                            , "path to the source data file")
            ("outdir,o", po::value<std::string>() -> default_value("./")
                                            , "path to the out data dir")
            ("mnum,m", po::value<std::size_t>() -> default_value(3)
                                            , "number of threads for display work")
            ("rnum,r", po::value<std::size_t>() -> default_value(3)
                                            , "number of threads for convolution work");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    try {
        std::cout << "begin" << std::endl;

        //if (!vm["worker"].as<bool>()) 
        {
            // ba::io_context ioContext;
            // ba::signal_set signals{ioContext, SIGINT, SIGTERM};
            // signals.async_wait([&](auto, auto) { std::cout << "ioContext stop" << std::endl;  ioContext.stop(); });
            // //ba::post(ioContext, [&] () { std::cout << "ioContext while" << std::endl; while(!ioContext.stopped()) { ; } std::cout << "ioContext while end" << std::endl; });
            // //ba::co_spawn(ioContext, [] () { std::cout << "ioContext while" << std::endl; while(1) { ; } }, ba::detached);

            // Server server;
            // server.listen(vm["port"].as<unsigned short>());

            // std::cout << "wait" << std::endl;


            // ioContext.run();
            // std::cout << "end" << std::endl;




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
            // std::filesystem::path input(vm["src"].as<std::string>().c_str());
            // std::filesystem::path output(vm["outdir"].as<std::string>().c_str());
            //worker
            mr->run(/*input, output*/);

            ioContext.run();
        }
        return EXIT_SUCCESS;




        // auto mNum = vm["mnum"].as<std::size_t>();
        // auto rNum = vm["rnum"].as<std::size_t>();

        // mapper_t mapper = [] (mapper_element_t& s) { return s; };
        // reducer_t reducer = [] (reducer_element_t res, reducer_element_t &el) {
        //     auto min = std::min(res.first.size(), el.first.size());
        //     el.second = 0;
        //     for (std::size_t i = 0; i < min; i++) {
        //         el.second = i + 1;
        //         if (res.first[i] != el.first[i]) {
        //             break;
        //         }
        //     }
        //     if (res.first.size() && el.second > res.second) {
        //         res.second = el.second;
        //     }
        //     if (el.second == res.first.size() && el.second < el.first.size()) {
        //         el.second += 1;
        //     }
        //     return res;
        // };

        // MapReduce mr(mNum, rNum, mapper, reducer);
        // std::filesystem::path input(vm["src"].as<std::string>().c_str());
        // std::filesystem::path output(vm["outdir"].as<std::string>().c_str());
        // mr.run(input, output);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}