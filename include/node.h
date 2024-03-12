#pragma once

#include "utils.h"

#include <memory>

class MapReduce;

class Node {
public:
    Node(std::shared_ptr<MapReduce> mr) : mr(mr) {}
    virtual ~Node() {}
    /** @brief  Returns MapReduce pointer
    */
    std::shared_ptr<MapReduce> getMr() {
        if (auto shpMr = mr.lock()) {
            return shpMr;
        }
        return nullptr;               
    }
protected:
    std::weak_ptr<MapReduce> mr;
};