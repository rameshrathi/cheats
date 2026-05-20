// ─────────────────────────────────────────────────────────────
//  npc_stub.cpp  –  UDP <-> Shared Memory bridge
//
//  Usage:  ./npc_stub <id> [peers_file]
//
//    id        : numeric ID
//                  - SHM segment : /shm_<id>
//                  - UDP listen  : port from peers_file or 20000+id
//    peers_file: optional; path to peers data file (default: peers.dat)
//
//  Compile:
//    g++ -std=c++11 -pthread -o npc_stub npc_stub.cpp UdpServer.cpp -lrt
// ─────────────────────────────────────────────────────────────

#include "SharedMemory.hpp"
#include "UdpServer.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <atomic>
#include <string>
#include <signal.h>

// ── globals ──────────────────────────────────────────────────
static std::atomic<bool> g_running{true};

static void on_signal(int) { g_running = false; }

// ── main ─────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <id> [peers_file]\n";
        return 1;
    }

    // ── parse arguments ──────────────────────────────────────
    const int      id          = std::stoi(argv[1]);
    std::string    peers_file  = (argc >= 3) ? argv[2] : "peers.dat";
    uint16_t       listen_port = static_cast<uint16_t>(20000 + id);

    struct PeerInfo {
        uint16_t id;
        std::string ip;
        uint16_t port;
    };
    std::vector<PeerInfo> peers;

    std::ifstream infile(peers_file);
    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto eq_pos = line.find('=');
            if (eq_pos != std::string::npos) {
                uint16_t node_id = static_cast<uint16_t>(std::stoi(line.substr(0, eq_pos)));
                std::string addr_part = line.substr(eq_pos + 1);
                auto colon_pos = addr_part.find(':');
                if (colon_pos != std::string::npos) {
                    std::string ip = addr_part.substr(0, colon_pos);
                    uint16_t port = static_cast<uint16_t>(std::stoi(addr_part.substr(colon_pos + 1)));
                    if (node_id == id) {
                        listen_port = port;
                    } else {
                        peers.push_back({node_id, ip, port});
                    }
                }
            }
        }
    } else {
        std::cerr << "[main] warning: could not open " << peers_file << "\n";
    }

    std::cout << "=== SHM-UDP bridge  id=" << id
              << "  shm=/shm_" << id
              << "  listen=:" << listen_port << " ===\n";

    // ── signal handling ──────────────────────────────────────
    ::signal(SIGINT,  on_signal);
    ::signal(SIGTERM, on_signal);

    // ── open shared memory ───────────────────────────────────
    SharedMemory shm(id);
    std::cout << "[main] SHM /shm_" << id
              << " opened (owner=" << shm.owner() << ")\n";

    // ── init UDP server ──────────────────────────────────────
    UdpServer server(listen_port, shm);
    std::cout << "[main] UDP socket bound to :" << listen_port << "\n";

    for (const auto& peer : peers) {
        if (!server.add_peer(peer.id, peer.ip, peer.port)) {
            std::cerr << "[main] invalid peer IP: " << peer.ip << "\n";
        }
    }

    // ── launch threads ───────────────────────────────────────
    server.start(g_running);

    std::cout << "[main] running – press Ctrl-C to stop\n";
    server.join();

    shm.close(shm.owner()); // unlink only if we created it
    std::cout << "[main] clean shutdown\n";
    return 0;
}