#include "mapreduce.h"

#include <thread>
#include <utility>
#include <iterator>
#include <numeric>

namespace {
using reduce_wrapper_t = std::function<reducer_element_t (reducer_element_t, reducer_element_t)>;

std::list<std::shared_ptr<Section>> dataSplit(const char* fileName, std::size_t mNum) {
    std::list<std::shared_ptr<Section>> sections;
    auto fileSize = std::filesystem::file_size(fileName);
    if (fileSize > 0) {
        std::size_t sectionSize = fileSize / mNum;
        std::ifstream istrm(fileName);
        if (!istrm) {
            std::cout << "failed to open " << fileName << std::endl;
            throw std::runtime_error("error read file");
        } else {
            if (sectionSize) {
                std::size_t begin = 0;
                std::size_t end = 0;
                for (std::size_t i = 0; i < mNum; i++) {
                    end = (i + 1)*sectionSize - 1;
                    std::string line;
                    istrm.seekg(end);
                    std::getline(istrm, line, '\n');
                    if (istrm.good()) {
                        end += line.size();
                    } else {
                        i = mNum;
                        end += line.size();
                    }
                    auto s = std::make_shared<Section>(begin, end);
                    sections.emplace_back(s);
                    //std::cout << "begin=" << begin << " end=" << end << std::endl;
                    std::string str(end - begin + 1, '\0');
                    istrm.seekg(begin);
                    istrm.read(&str[0], end - begin);
                    begin = end + 1;
                }
            }
        }
    }
    return sections;
}

void mapFhase(const char* fileName, std::shared_ptr<Section> s, mapper_t mapper, std::size_t index) {
    //std::cout << std::this_thread::get_id() << " " << s->begin << " " << s->end << std::endl;
    std::ifstream istrm(fileName);
    if (!istrm) {
        std::cout << "failed to open " << fileName << std::endl;
        throw std::runtime_error("error read file");
    } else {
        std::vector<std::string> result;
        std::string str;
        istrm.seekg(s->begin);
        while ((std::size_t)istrm.tellg() <= s->end && std::getline(istrm, str, '\n')) {
            result.emplace_back(mapper(str));
        }
        std::sort(result.begin(), result.end());
        {
            std::ofstream ostrm(std::format("{}.map", index));
            ostrm << result.size() << "\n";
            std::for_each(result.begin(), result.end(), [&ostrm](auto &val){
                ostrm << val << "\n";
            });
        }
    }
}

void reduceFhase(reducer_t reducer, const char* outDir, std::size_t index) {
    if (std::filesystem::exists(std::format("{}.shuffle", index))) {
        std::ifstream istrm(std::format("{}.shuffle", index));
        std::ofstream ostrm(std::format("{}{}.reduce", outDir, index));
        reduce_wrapper_t reduceWrapper = [&ostrm, &reducer] (reducer_element_t res, reducer_element_t el) {
                                                auto returnVal = reducer(res, el);
                                                if (res.first.size()) {
                                                    ostrm << returnVal.first << " " << returnVal.second << std::endl;
                                                }
                                                return el;
                                            };
        auto res = std::accumulate((std::istream_iterator<DataForRead>(istrm))
                                    , std::istream_iterator<DataForRead>()
                                    , reducer_element_t {"", 0}
                                    , reduceWrapper);
        ostrm << res.first << " " << res.second << std::endl;
        std::filesystem::remove(std::format("{}.shuffle", index));
    }
}

void shuffle(std::size_t mNum, std::size_t rNum) {
    std::size_t linesCount {0};
    std::vector<File> files;
    for (std::size_t i = 0; i < mNum; i++) {
        files.emplace_back(File(i));
        linesCount += files.back().linesCount;
    }
    std::size_t border = linesCount / rNum;
    std::size_t count {0};
    std::size_t reduceIndex {0};
    //std::cout << "linesCount=" << linesCount << " rNum=" << rNum << " border=" << border << std::endl;
    if (linesCount) {
        std::ofstream ostrm(std::format("{}.shuffle", reduceIndex));
        std::string lastStr;
        std::size_t minI = 0;
        bool isEnd = false;
        while (!isEnd) {
            for (std::size_t i = 0; i < mNum; i++) {
                if (minI == i) continue;
                if (files[i].end) continue;
                if (files[i].curStr < files[minI].curStr) {
                    minI = i;
                }
            }

            {
                //border
                bool newKey = true;
                if (files[minI].curStr.size() && lastStr.size()) {
                    newKey = files[minI].curStr[0] != lastStr[0];
                }
                if (border && border < count + 1 && newKey) {
                    count = 0;
                    if (reduceIndex + 1 < rNum) {
                        ostrm.close();
                        reduceIndex++;
                        ostrm.open(std::format("{}.shuffle", reduceIndex));
                    }
                }
                ostrm << files[minI].curStr << "\n";
                lastStr = files[minI].curStr;
                count++;
            }
            files[minI].next();
            {
                isEnd = true;
                for (std::size_t i = 0; i < mNum; i++) {
                    if (files[i].end) continue;
                    isEnd = false;
                    minI = i;
                    break;
                }
            }
        }     
    }
    for (std::size_t i = 0; i < mNum; i++) {
        std::filesystem::remove(std::format("{}.map", i));
    }   
}
} //namespace

std::istream& operator >> (std::istream& in, DataForRead& output) {
    output.second = 0;
    std::getline(in, output.first, '\n');
    return in;
}

void MapReduce::run(const std::filesystem::path& input, const std::filesystem::path& output) {
    //split
    std::list<std::shared_ptr<Section>> sections = dataSplit(input.relative_path().c_str(), mappersCount);
    mappersCount = sections.size();

    //map
    {
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < mappersCount; ++i) {
            auto it = sections.begin();
            std::advance(it, i);
            auto s = *it;
            std::thread th = std::thread(mapFhase, input.relative_path().c_str(), s, mapper, i);
            threads.emplace_back(std::move(th));
        }
        std::for_each(threads.begin(), threads.end(), [](auto& it) { it.join(); });
    }

    //shuffle
    shuffle(mappersCount, reducersCount);

    //reduce
    {
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < reducersCount; ++i) {
            std::thread th = std::thread(reduceFhase, reducer, output.relative_path().c_str(), i);
            threads.emplace_back(std::move(th));
        }
        std::for_each(threads.begin(), threads.end(), [](auto& it) { it.join(); });
    }
}
