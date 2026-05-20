#pragma once

#include "SharedMemory.hpp"
#include <atomic>
#include <thread>
#include <string>
#include <unordered_map>
#include <netinet/in.h>

class UdpServer {
public:
    UdpServer(uint16_t listen_port, SharedMemory& shm);
    ~UdpServer();

    // Non-copyable
    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

    bool add_peer(uint16_t node_id, const std::string& peer_ip, uint16_t peer_port);
    void start(std::atomic<bool>& running_flag);
    void join();

private:
    void thread_shm_to_udp(std::atomic<bool>& running_flag);
    void thread_udp_to_shm(std::atomic<bool>& running_flag);

    SharedMemory& shm_;
    int sock_fd_;
    uint16_t listen_port_;
    
    std::unordered_map<uint16_t, sockaddr_in> peers_;

    std::thread t_shm_udp_;
    std::thread t_udp_shm_;
};