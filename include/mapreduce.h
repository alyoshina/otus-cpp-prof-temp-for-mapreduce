#pragma once

#include <list>
#include <iostream>
#include <functional>
#include <format>
#include <filesystem>
#include <fstream>

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

class MapReduce {
public:
    MapReduce(std::size_t m, std::size_t r, mapper_t mapper, reducer_t reducer)
            : mappersCount(m)
            , reducersCount(r)
            , mapper(mapper)
            , reducer(reducer) {

            }
    void run(const std::filesystem::path& input, const std::filesystem::path& output);

private:
    std::size_t mappersCount;
    std::size_t reducersCount;
    mapper_t mapper;
    reducer_t reducer;
};