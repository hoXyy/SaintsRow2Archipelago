#include <ws2tcpip.h>
#include <json.hpp>
#include <sr2ap/ArchipelagoTcpClient.hpp>
#include <sr2ap/Logger.hpp>
#include <stdexcept>
#include <string>
#include <utility>

using json = nlohmann::json;

namespace sr2ap {
    namespace {
        constexpr auto reconnectDelay = std::chrono::seconds{2};
        constexpr std::size_t maxIncomingBuffer = 64 * 1024;

        std::runtime_error WinsockError(const std::string_view operation) {
            return std::runtime_error{std::string{operation} +
                                      " failed, WSA error=" + std::to_string(WSAGetLastError())};
        }
    }  // namespace

    ArchipelagoTcpClient::ArchipelagoTcpClient(std::uint16_t port, MessageHandler messageHandler)
        : port_{port}, messageHandler_{std::move(messageHandler)} {
        WSADATA winsockData{};

        if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0) {
            throw WinsockError("WSAStartup");
        };
    };

    ArchipelagoTcpClient::~ArchipelagoTcpClient() {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
        };

        WSACleanup();
    };

    bool ArchipelagoTcpClient::IsConnected() const noexcept {
        return state_ == State::connected;
    }

    void ArchipelagoTcpClient::Poll() {
        switch (state_) {
            case State::connected:
                PollConnected();
                break;
            case State::disconnected:
                if (std::chrono::steady_clock::now() >= nextConnectionAttempt_) {
                    BeginConnect();
                }
                break;
            case State::connecting:
                PollConnecting();
                break;
        }
    }

    void ArchipelagoTcpClient::BeginConnect() {
        socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_ == INVALID_SOCKET) {
            Disconnect("Failed to create socket");
            return;
        }

        u_long nonBlocking = 1;

        if (ioctlsocket(socket_, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
            Disconnect("Failed to enable non-blocking mode");
            return;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port_);

        if (const int result = connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)); result == 0) {
            state_ = State::connected;
            LogInfo("Network", "Connected to client");
            const json message = {{"type", "hello"}, {"protocol", 2}, {"game", "Saints Row 2"}};
            SendLine(message.dump());
            return;
        }

        if (const int error = WSAGetLastError(); error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
            state_ = State::connecting;
            return;
        }

        Disconnect("Failed to connect");
    }

    void ArchipelagoTcpClient::PollConnected() {
        FlushOutgoing();

        if (state_ == State::connected) {
            ProcessIncoming();
        }
    }

    void ArchipelagoTcpClient::PollConnecting() {
        fd_set writable;
        FD_ZERO(&writable);
        FD_SET(socket_, &writable);

        fd_set exceptional;
        FD_ZERO(&exceptional);
        FD_SET(socket_, &exceptional);

        constexpr timeval timeout{};
        const int result = select(0, nullptr, &writable, &exceptional, &timeout);

        if (result == SOCKET_ERROR) {
            Disconnect("select failed while connecting");
            return;
        }

        if (result == 0) {
            return;
        }

        int socketError{};
        int length = sizeof(socketError);

        if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &length) == SOCKET_ERROR ||
            socketError != 0) {
            Disconnect("connection refused");
            return;
        }

        state_ = State::connected;
        LogInfo("Network", "Connected to AP client");
        const json helloMessage = {{"type", "hello"}, {"protocol", 2}, {"game", "Saints Row 2"}};
        SendLine(helloMessage.dump());
    }

    void ArchipelagoTcpClient::ProcessIncoming() {
        char buffer[4096];

        while (true) {
            const int received = recv(socket_, buffer, sizeof(buffer), 0);

            if (received > 0) {
                incoming_.append(buffer, static_cast<std::size_t>(received));

                if (incoming_.size() > maxIncomingBuffer) {
                    Disconnect("Max buffer size limit reached");
                    return;
                }

                std::size_t newline{};
                while ((newline = incoming_.find('\n')) != std::string::npos) {
                    const std::string message = incoming_.substr(0, newline);
                    incoming_.erase(0, newline + 1);
                    LogDebug("Network", "Received: " + message);
                    if (messageHandler_) {
                        messageHandler_(message);
                    }
                }

                continue;
            }

            if (received == 0) {
                Disconnect("AP Client closed connection");
                return;
            }

            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                return;
            }

            Disconnect("receive failed");
            return;
        }
    }

    void ArchipelagoTcpClient::SendLine(const std::string_view message) {
        outgoing_.append(message);
        outgoing_.push_back('\n');
    }

    void ArchipelagoTcpClient::FlushOutgoing() {
        while (!outgoing_.empty()) {
            const int sent = send(socket_, outgoing_.data(), static_cast<int>(outgoing_.size()), 0);

            if (sent > 0) {
                outgoing_.erase(0, static_cast<std::size_t>(sent));
                continue;
            }

            if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
                return;
            }

            Disconnect("send failed");
            return;
        }
    }

    void ArchipelagoTcpClient::Disconnect(const std::string_view reason) {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }

        if (state_ != State::disconnected) {
            LogWarning("Network", std::string{reason});
        }

        state_ = State::disconnected;
        incoming_.clear();
        outgoing_.clear();
        nextConnectionAttempt_ = std::chrono::steady_clock::now() + reconnectDelay;
    }
}  // namespace sr2ap
