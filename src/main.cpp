#include "mapreduce.h"

#include <boost/asio/thread_pool.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main([[maybe_unused]]int argc, [[maybe_unused]]char** argv) {

    //processing сommand line argument
    po::options_description desc {"Options"};
    desc.add_options()
            ("help,h", "MapReduce task launch system")
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
        auto mNum = vm["mnum"].as<std::size_t>();
        auto rNum = vm["rnum"].as<std::size_t>();

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

        MapReduce mr(mNum, rNum, mapper, reducer);
        std::filesystem::path input(vm["src"].as<std::string>().c_str());
        std::filesystem::path output(vm["outdir"].as<std::string>().c_str());
        mr.run(input, output);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}