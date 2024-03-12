#pragma once


#include "client.h"
#include "thread_pool.h"
#include "node.h"

#include <iostream>
#include <set>

//class MapReduce;

/**
*  @brief  MainNode is server for the WorkerNode. 
* Сonnects clients and WorkerNodes, sends data from clients to WorkerNodes for executing the @map and @reduce functions
* 
*/
class MainNode : public ThreadPool, public Node {
public:
    MainNode(std::shared_ptr<MapReduce> mr)
    : ThreadPool(2)
    , Node(mr)
    {
    }
    ~MainNode() {
        std::cout << "delete MainNode" << std::endl;
    };
    void listen(uint16_t port) {
        std::cout << "listen" << std::endl;
        ba::co_spawn(threadPool, startAccept(port), ba::detached);
    }
    void addClient(std::shared_ptr<Client> client) { clients.insert(client); std::cout << "insert client" << client << std::endl; }
    void addWorker(std::shared_ptr<Client> client) { workers.insert(client); std::cout << "insert worker" << client << std::endl; }
    void deleteClient(std::shared_ptr<Client> client) { clients.erase(client); std::cout << "delete client" << client << std::endl;}
    void deleteWorker(std::shared_ptr<Client> client) { workers.erase(client); std::cout << "delete worker" << client << std::endl;}

    ba::awaitable<void> dataProcessing(std::shared_ptr<Client> client, std::string &&path);

private:
    //std::weak_ptr<MapReduce> mr;
    std::set<std::shared_ptr<Client>> clients;
    std::set<std::shared_ptr<Client>> workers;
    /** @brief  Initiates an asynchronous accept operation to wait for a new connection 
    */
    ba::awaitable<void> startAccept(uint16_t port);
    // void stop() {
    //     mainNode.leave(shared_from_this());
    //     socket.close();
    // }

};