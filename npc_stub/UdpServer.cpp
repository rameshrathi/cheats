#include "UdpServer.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cerrno>

UdpServer::UdpServer(uint16_t listen_port, SharedMemory& shm)
    : shm_(shm), sock_fd_(-1), listen_port_(listen_port)
{
    sock_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd_ < 0) throw std::runtime_error("socket() failed");

    int yes = 1;
    ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Non-blocking so threads can check running flag periodically
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    ::setsockopt(sock_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(listen_port_);

    if (::bind(sock_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(sock_fd_);
        throw std::runtime_error("bind() failed on port " + std::to_string(listen_port_));
    }
}

UdpServer::~UdpServer() {
    if (sock_fd_ >= 0) {
        ::close(sock_fd_);
    }
}

bool UdpServer::add_peer(uint16_t node_id, const std::string& peer_ip, uint16_t peer_port) {
    sockaddr_in peer_addr{};
    std::memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port   = htons(peer_port);
    if (::inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr) != 1) {
        return false;
    }
    peers_[node_id] = peer_addr;
    return true;
}

void UdpServer::start(std::atomic<bool>& running_flag) {
    t_shm_udp_ = std::thread(&UdpServer::thread_shm_to_udp, this, std::ref(running_flag));
    t_udp_shm_ = std::thread(&UdpServer::thread_udp_to_shm, this, std::ref(running_flag));
}

void UdpServer::join() {
    if (t_shm_udp_.joinable()) t_shm_udp_.join();
    if (t_udp_shm_.joinable()) t_udp_shm_.join();
}

void UdpServer::thread_shm_to_udp(std::atomic<bool>& running_flag) {
    char buf[SharedMemory::BUFFER_SIZE];

    std::cout << "[shm→udp] thread started (" << peers_.size() << " peers configured)\n";

    while (running_flag) {
        std::size_t n = shm_.write_pending();
        if (n > 0) {
            n = shm_.read(buf, n);
            if (n > 0) {
                shm_.clear_write();
                
                if (n >= 2) {
                    uint16_t target_id;
                    std::memcpy(&target_id, buf, 2);

                    if (target_id == 0xFFFF) {
                        for (const auto& peer : peers_) {
                            ::sendto(sock_fd_, buf, n, 0,
                                     reinterpret_cast<const sockaddr*>(&peer.second),
                                     sizeof(peer.second));
                        }
                        std::cout << "[shm→udp] broadcasted " << n
                                  << " bytes to " << peers_.size() << " peers\n";
                    } else {
                        auto it = peers_.find(target_id);
                        if (it != peers_.end()) {
                            ssize_t sent = ::sendto(sock_fd_, buf, n, 0,
                                                    reinterpret_cast<const sockaddr*>(&it->second),
                                                    sizeof(it->second));
                            if (sent < 0)
                                std::perror("[shm→udp] sendto");
                            else
                                std::cout << "[shm→udp] forwarded " << sent
                                          << " bytes to peer " << target_id << "\n";
                        } else {
                            std::cout << "[shm→udp] SHM data ready but target peer "
                                      << target_id << " not found – dropping " << n << " bytes\n";
                        }
                    }
                } else {
                    std::cout << "[shm→udp] payload too small (" << n << " bytes) to contain target id – dropping\n";
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "[shm→udp] thread exiting\n";
}

void UdpServer::thread_udp_to_shm(std::atomic<bool>& running_flag) {
    char buf[SharedMemory::BUFFER_SIZE];
    sockaddr_in sender{};
    socklen_t   slen = sizeof(sender);

    std::cout << "[udp→shm] thread started, listening on :"
              << listen_port_ << "\n";

    while (running_flag) {
        ssize_t n = ::recvfrom(sock_fd_, buf, sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&sender), &slen);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
            std::perror("[udp→shm] recvfrom");
            continue;
        }

        char peer_str[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &sender.sin_addr, peer_str, sizeof(peer_str));
        std::cout << "[udp→shm] received " << n << " bytes from "
                  << peer_str << ":" << ntohs(sender.sin_port) << "\n";

        std::size_t written = shm_.write_to_read_buf(buf, static_cast<std::size_t>(n));
        std::cout << "[udp→shm] wrote " << written << " bytes into SHM read-buf\n";
    }
    std::cout << "[udp→shm] thread exiting\n";
}