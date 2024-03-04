#pragma once

#include "server.h"
//#include "ssh.h"

#include <list>
#include <iostream>
#include <functional>
#include <format>
#include <filesystem>
#include <fstream>
#include <memory>

struct Section {
    Section(std::size_t b, std::size_t e) : begin(b), end(e) {}
    std::size_t begin;
    std::size_t end;
};

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

class MapReduce : public std::enable_shared_from_this<MapReduce>{
public:
    MapReduce(/*std::size_t m, std::size_t r, */mapper_t mapper, reducer_t reducer)
            : 
            // mappersCount(m)
            // , reducersCount(r)
            mapper(mapper)
            , reducer(reducer) {

            }
    void run(const std::filesystem::path& input, const std::filesystem::path& output);
    void run(bool worker = false, uint16_t port = 9000) {
        if (!worker) {
            mainNode = std::make_shared<Server>(shared_from_this());
            mainNode->listen(port);
        }
    }

    template<typename T>
    ba::awaitable<std::list<std::shared_ptr<Section>>> dataSplitConn(std::shared_ptr<T> conn) {
        auto initiate = [this, conn]<typename Handler>(Handler&& handler) mutable {
            ba::post(CreateConnection::init().getThreadPool()//conn->threadPool
                        , [handler = std::forward<Handler>(handler), this, conn]() mutable {
                handler(this->_dataSplitConn<T>(conn, 3));
            });
        };
        return ba::async_initiate<decltype(ba::use_awaitable), void(std::list<std::shared_ptr<Section>>)>(initiate, ba::use_awaitable);
    }

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
                        //std::getline(istrm, line, '\n');
                        // conn.getline(line);
                        // if (istrm.good()) {
                        //     end += line.size();
                        // } else {
                        //     i = mNum;
                        //     end += line.size();
                        // }
                        ;
                        if (conn->getline(line)) {
                            end += line.size();
                        } else {
                            i = mNum;
                            end += line.size();
                        }
                        auto s = std::make_shared<Section>(begin, end);
                        sections.emplace_back(s);
                        std::cout << "begin=" << begin << " end=" << end << std::endl;
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




    // ba::awaitable<std::list<std::shared_ptr<Section>>> dataSplitSshAsync(const char* fileName, std::size_t mNum) {
    //     auto initiate = [this, fileName, mNum]<typename Handler>(Handler&& handler) mutable {
    //         ba::post(client->getParserContext().get_executor()
    //                     , [handler = std::forward<Handler>(handler), this, client, fileName, mNum]() mutable {
    //             handler(this->dataSplitSsh(fileName, mNum));
    //         });
    //     };
    //     return ba::async_initiate<decltype(ba::use_awaitable), void(std::list<std::shared_ptr<Section>>)>(initiate, ba::use_awaitable);
    // }
    // std::list<std::shared_ptr<Section>> dataSplitSsh(const char* fileName, std::size_t mNum) {
    //     std::list<std::shared_ptr<Section>> sections;
    //     auto fileSize = std::filesystem::file_size(fileName);
    //     if (fileSize > 0) {
    //         std::size_t sectionSize = fileSize / mNum;
    //         std::ifstream istrm(fileName);
    //         if (!istrm) {
    //             std::cout << "failed to open " << fileName << std::endl;
    //             throw std::runtime_error("error read file");
    //         } else {
    //             if (sectionSize) {
    //                 std::size_t begin = 0;
    //                 std::size_t end = 0;
    //                 for (std::size_t i = 0; i < mNum; i++) {
    //                     end = (i + 1)*sectionSize - 1;
    //                     std::string line;
    //                     istrm.seekg(end);
    //                     std::getline(istrm, line, '\n');
    //                     if (istrm.good()) {
    //                         end += line.size();
    //                     } else {
    //                         i = mNum;
    //                         end += line.size();
    //                     }
    //                     auto s = std::make_shared<Section>(begin, end);
    //                     sections.emplace_back(s);
    //                     //std::cout << "begin=" << begin << " end=" << end << std::endl;
    //                     std::string str(end - begin + 1, '\0');
    //                     istrm.seekg(begin);
    //                     istrm.read(&str[0], end - begin);
    //                     begin = end + 1;
    //                 }
    //             }
    //         }
    //     }
    //     return sections;
    // }


private:
    std::size_t mappersCount;
    std::size_t reducersCount;
    mapper_t mapper;
    reducer_t reducer;
    std::shared_ptr<Server> mainNode;
};