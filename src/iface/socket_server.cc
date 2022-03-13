#include "socket_server.hh"

#include <stringf.hh>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Interface {

// ============================================================================

SocketServer::SocketServer (size_t a_ListenPort, size_t a_MaxClients) :
    Worker (),
    m_ListenPort (a_ListenPort),
    m_MaxClients (a_MaxClients)
{
    // Create the logger
    std::string loggerName = stringf("server [%d]", (uint64_t)m_ListenPort);
    m_Logger = spdlog::get(loggerName);
    if (!m_Logger) m_Logger = spdlog::stderr_color_mt(loggerName);
}

// ============================================================================

bool SocketServer::start () {

    m_Logger->info("Starting TCP server at {}...", m_ListenPort);

    // Create the listen socket
    m_Socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_Socket < 1) {
        m_Logger->error("Error creating the listen socket!");
        return false;
    }

    // Bind the socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(m_ListenPort);

    if (bind(m_Socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        m_Logger->error("Error binding the listen socket!");

        close(m_Socket);
        m_Socket = -1;
        return false;
    }

    // Start listening
    listen(m_Socket, 5); // FIXME: Max waiting queue

    return Worker::start();
}

void SocketServer::stop () {  

    m_Logger->info("Stopping TCP server...");

    // Stop the worker
    Worker::stop();

    // Close the listen socket
    if (m_Socket != -1) {
        close(m_Socket);
        m_Socket = -1;
    }
}

// ============================================================================

int SocketServer::loop () {

    bool canSleep = true;

    // Accept new connections
    if (m_Clients.size() < m_MaxClients) {
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(struct sockaddr_in);

        // Try accepting a connection
        int fd = accept4(m_Socket, (struct sockaddr*)&addr, &addrLen, SOCK_NONBLOCK);
        if (fd >= 0) {
            m_Logger->info("Client {} connected, IP {}", fd, inet_ntoa(addr.sin_addr));

            std::lock_guard<std::mutex> lock(m_Lock);
            m_Clients.set(fd, std::shared_ptr<Client>(new Client(fd)));

            canSleep = false;
        }
        // An error occurred
        else if(errno != EAGAIN && errno != EWOULDBLOCK) {
            m_Logger->error("accept() Error: {}", strerror(errno));
        }
    }

    // Handle client connections
    char buffer [256];
    int  len;

    m_Lock.lock();

    for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ) {
        auto&  client = itr->second;

        // Read
        len = ::read(client->socket, buffer, sizeof(buffer));

        // Graceful disconnect or error
        if (len == 0 || (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK)){
            if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                m_Logger->error("read() Error: {}", strerror(errno));
            }

            m_Logger->info("Disconnecting client {}", client->socket);

            shutdown(client->socket, SHUT_RDWR);
            close(client->socket);

            itr = m_Clients.del(itr);
            continue;
        }
        // Complete receive
        else if (len > 0) {

            // Put the received data to the buffer
            auto& rxData = client->rxData;
            rxData.insert(rxData.end(), buffer, buffer + len);

            // Try getting a full line
            processReceivedData(client.get());

            canSleep = false;
        }

        // Get data from client's queue. Concatenate lines and separate them
        // with '\r\n'
        auto& txData = client->txData;
        while (!client->txQueue.empty()) {
            auto& data = client->txQueue.front();

            txData.insert(txData.end(), data.begin(), data.end());
            txData.insert(txData.end(), {'\r', '\n'});

            client->txQueue.pop();
        }

        // Send data
        if (!txData.empty()) {

            // Try sending
            len = ::send(client->socket, txData.data(), txData.size(), 0);
        
            // Graceful disconnect or error
            if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                m_Logger->error("send() Error: {}", strerror(errno));

                m_Logger->info("Disconnecting client {}", client->socket);

                shutdown(client->socket, SHUT_RDWR);
                close(client->socket);

                itr = m_Clients.del(itr);
                continue;
            }
            // Only part of data was sent
            else if (len < (int)txData.size()) {
                txData.assign(txData.begin() + len, txData.end());
                canSleep = false;
            }
            // Complete send
            else if (len == (int)txData.size()) {
                txData.clear();
                canSleep = false;
            }
        }

        // Next client
        ++itr;
    }

    m_Lock.unlock();

    // Sleep to save CPU cycles
    if (canSleep) {
        usleep(1000); // 1ms
    }

    return 0;
}

// ============================================================================

void SocketServer::processReceivedData(Client* a_Client) {
    auto& data = a_Client->rxData;

    // Find line terminators and extract full lines
    while (1) {

        // Get next newline
        ssize_t nlPos = -1;
        for (size_t i=0; i<data.size(); ++i) {
            if (data[i] == '\n') {
                nlPos = i;
                break;
            }
        }

        // No more newlines
        if (nlPos == -1) {
            break;
        }

        // Extract line
        std::string line (data.begin(), data.begin() + nlPos);

        // Remove trailing '\r' if any
        size_t crPos = line.rfind('\r');
        if (crPos != std::string::npos) {
            line.erase(crPos);
        }

        // Push it to the queue
        a_Client->rxQueue.push(line);

        // Remove it from the buffer
        data.assign(data.begin() + nlPos + 1, data.end());
    }
}

// ============================================================================

void SocketServer::sendLines (int a_ClientId, const std::vector<std::string>& a_Lines) {
    std::lock_guard<std::mutex> lock(m_Lock);

    // Do not have that client
    if (!m_Clients.has(a_ClientId)) {
        m_Logger->warn("Client {} not found!", a_ClientId);
        return;
    }

    // Put lines to the queue
    auto& client = m_Clients.get(a_ClientId);
    for (auto& line : a_Lines) {
        client->txQueue.push(line);
    }
}

std::unordered_map<int, std::vector<std::string>> SocketServer::getLines () {
    std::lock_guard<std::mutex> lock(m_Lock);

    // Process all clients
    std::unordered_map<int, std::vector<std::string>> allLines;

    for (auto& it : m_Clients) {
        int clientId = it.first;
        auto& client = it.second;

        // Get lines
        std::vector<std::string> lines;
        while (!client->rxQueue.empty()) {
            auto line = client->rxQueue.front();
            lines.push_back(line);
            client->rxQueue.pop();
        }

        // Store them
        allLines[clientId] = lines;
    }

    return allLines;
}

// ============================================================================
}; // Interface
