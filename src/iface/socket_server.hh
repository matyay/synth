#ifndef INTERFACE_SOCKET_SERVER_HH
#define INTERFACE_SOCKET_SERVER_HH

#include <utils/dict.hh>
#include <utils/worker.hh>

#include <spdlog/spdlog.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <memory>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Interface {

// ============================================================================

class SocketServer : public Worker {
public:

    /// Constructor
     SocketServer (size_t a_ListenPort, size_t a_MaxClients = 1);
    ~SocketServer () override {};

    /// Start
    bool start () override;
    /// Stop
    void stop  () override;

    /// Send text lines to a client
    void sendLines (int a_ClientId, const std::vector<std::string>& a_Lines);
    /// Read text lines from all clients
    std::unordered_map<int, std::vector<std::string>> getLines ();

protected:

    /// The worker loop
    int loop () override;

    /// Client context
    struct Client {

        /// Communication socket
        int socket;

        /// Receive queue
        std::queue<std::string> rxQueue;
        /// Transmitt queue
        std::queue<std::string> txQueue;

        /// Received data
        std::vector<uint8_t> rxData;
        /// To-be-transmitted data
        std::vector<uint8_t> txData;

        /// Constructor
        Client (int a_Socket) : socket(a_Socket) {}
    };

    /// Logger
    std::shared_ptr<spdlog::logger> m_Logger;

    /// Listen port
    size_t m_ListenPort;
    /// Max clients
    size_t m_MaxClients;

    /// Listen socket
    int m_Socket = -1;
    /// Clients
    Dict<int, std::shared_ptr<Client>> m_Clients;

    /// Synchronization mutex
    std::mutex m_Lock;

    /// Processes received data. Splits the character stream into lines and
    /// stores them in the client's receive queue
    void static processReceivedData(Client* a_Client);
};

// ============================================================================

}; // Interface
#endif // INTERFACE_SOCKET_SERVER_HH
 
