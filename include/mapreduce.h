#pragma once

#include "main_node.h"
#include "worker_node.h"
#include "create_connection.h"

#include <list>
#include <iostream>
#include <functional>
#include <format>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

/**
*  @brief  Section for the file.
*  @param[in]  b is section begin
*  @param[in]  e is section end
* 
*/
struct Section {
    Section() {}
    Section(std::size_t b, std::size_t e) : begin(b), end(e) {}
    std::size_t begin;
    std::size_t end;
    // std::vector<char> serialize() {
    //     std::vector<char> data;

    //     return data;
    // }
    // auto serialize(/*std::vector<char> &data*/) {
    //     return ba::buffer(this, sizeof(Section));
    // }
    // void deserialize(ba::buffer buf) {
    //     Section*p = ba::buffer_cast<Section*>(buf);
    // }
};
std::ostream &operator<<(std::ostream &out, Section const &s);
std::istream &operator>>(std::istream &in, Section &s);

struct File {
    File(std::size_t i) : index(i), istrm(std::format("{}.map", i)) {
        next();
        linesCount = std::stoi(curStr);
        next();
    }
    std::size_t index;
    std::ifstream istrm;
    std::size_t linesCount;
    bool end {false};
    std::string curStr;
    bool next() {
        return end = !(bool(std::getline(istrm, curStr, '\n'))); 
    }
};

using mapper_element_t = std::string;
using mapper_t = std::function<mapper_element_t(mapper_element_t&)>;
using reducer_element_t = std::pair<std::string, std::size_t>;
using reducer_t = std::function<reducer_element_t (reducer_element_t, reducer_element_t&)>;

struct DataForRead : public reducer_element_t { };
std::istream& operator >> (std::istream& in, DataForRead& output);

/**
*  @brief  For distributed data computing.
*  @param[in]  mapper is data preprocessing function
*  @param[in]  reducer is final data calculation function
* 
*/
class MapReduce : public std::enable_shared_from_this<MapReduce>{
public:
    MapReduce(/*std::size_t m, std::size_t r, */mapper_t mapper, reducer_t reducer)
            : 
            // mappersCount(m)
            // , reducersCount(r)
            mapper(mapper)
            , reducer(reducer) {

            }
    //void run(const std::filesystem::path& input, const std::filesystem::path& output);

    /**
    *  @brief  Running the main node or worker node.
    *  @param[in]  worker is true. If worker argument is equal to true then running the worker node
    * , else running the main node
    *  @param[in]  port is listening port of main node, not used for worker node
    * 
    */
    void run(bool worker, uint16_t port = 9000, std::string ip = "127.0.0.1") {
        if (worker) {
            auto workerNode = std::make_shared<WorkerNode>(shared_from_this());
            node = workerNode;
            workerNode->connect(ip, port);
            
        } else {
            auto mainNode = std::make_shared<MainNode>(shared_from_this());
            node = mainNode;
            mainNode->listen(port);
        }
    }

    /**
    *  @brief  Wrapper for asynchronous execution of the data splitting into sections function, used in the main node.
    *  @param[in]  conn is shared pointer of connection to remote data
    *  @return  list of section pointers for data
    * 
    */
    template<typename T>
    ba::awaitable<std::list<std::shared_ptr<Section>>> dataSplitConn(std::shared_ptr<T> conn) {
        auto initiate = [this, conn]<typename Handler>(Handler&& handler) mutable {
            ba::post(CreateConnection::init().getThreadPool()
                        , [handler = std::forward<Handler>(handler), this, conn]() mutable {
                handler(this->_dataSplitConn<T>(conn, 3));
            });
        };
        return ba::async_initiate<decltype(ba::use_awaitable), void(std::list<std::shared_ptr<Section>>)>(initiate, ba::use_awaitable);
    }

    template<typename T>
    ba::awaitable<std::vector<std::string>> mapFhase(std::shared_ptr<T> conn, std::shared_ptr<Section> s) {
        auto initiate = [this, conn, s]<typename Handler>(Handler&& handler) mutable {
            ba::post(CreateConnection::init().getThreadPool()
                        , [handler = std::forward<Handler>(handler), this, conn, s]() mutable {
                handler(this->_mapFhase<T>(conn, s));
            });
        };
        return ba::async_initiate<decltype(ba::use_awaitable), void(std::vector<std::string>)>(initiate, ba::use_awaitable);
    }

private:
    std::size_t mappersCount;
    std::size_t reducersCount;
    mapper_t mapper;
    reducer_t reducer;
    std::shared_ptr<Node> node;

    template<typename T>
    auto _mapFhase(std::shared_ptr<T> conn, std::shared_ptr<Section> s) {
        std::vector<std::string> result;
        std::string str;
        conn->seek(s->begin);
        while ((std::size_t)conn->tell() <= s->end && conn->getline(str, '\n')) {
            result.emplace_back(mapper(str));
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    /**
    *  @brief  Function of splitting data into sections, used in the main node.
    *  @param[in]  conn is shared pointer of connection to remote data
    *  @param[in]  mNum is mappers count
    *  @return  list of section pointers for data
    * 
    */
    template<typename T>
    std::list<std::shared_ptr<Section>> _dataSplitConn(std::shared_ptr<T> conn, std::size_t mNum) {
        std::list<std::shared_ptr<Section>> sections;
        auto fileSize = conn->getSize();
        std::cout << "fileSize=" << fileSize << std::endl;
        if (fileSize > 0) {
            std::size_t sectionSize = fileSize / mNum;
            std::cout << "sectionSize=" << sectionSize << std::endl;
            if (!sectionSize) {
                sectionSize = fileSize - 1;
                mNum = 1;
            }
            {
                if (sectionSize) {
                    std::size_t begin = 0;
                    std::size_t end = 0;
                    for (std::size_t i = 0; i < mNum; i++) {
                        end = (i + 1)*sectionSize - 1;
                        std::string line;
                        conn->seek(end);
                        std::cout << "seec=" << end << std::endl;
                        if (conn->getline(line)) {
                            end += line.size() ? line.size() - 1 : 0;
                        } else {
                            i = mNum;
                            end += line.size();
                        }
                        std::cout << "line=" << line << std::endl;
                        auto s = std::make_shared<Section>(begin, end);
                        sections.emplace_back(s);
                        std::cout << "---------- begin=" << begin << " end=" << end << std::endl;
                        ///std::string str(end - begin + 1, '\0');
                        conn->seek(begin);
                        ///conn.read(&str[0], end - begin);
                        begin = end + 1;
                    }
                }
            }
        }
        return sections;
    }
};