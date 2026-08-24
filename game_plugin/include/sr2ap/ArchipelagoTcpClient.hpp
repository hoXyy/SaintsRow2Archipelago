#pragma once

#include <winsock2.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace sr2ap {
    class ArchipelagoTcpClient {
       public:
        using MessageHandler = std::function<void(std::string_view)>;

        explicit ArchipelagoTcpClient(std::uint16_t port, MessageHandler messageHandler = {});
        ~ArchipelagoTcpClient();

        ArchipelagoTcpClient(const ArchipelagoTcpClient&) = delete;
        ArchipelagoTcpClient& operator=(const ArchipelagoTcpClient&) = delete;

        void Poll();
        void SendLine(std::string_view message);

        [[nodiscard]] bool IsConnected() const noexcept;

       private:
        enum class State {
            disconnected,
            connected,
            connecting,
        };

        void BeginConnect();
        void PollConnecting();
        void PollConnected();
        void Disconnect(std::string_view reason);
        void FlushOutgoing();
        void ProcessIncoming();

        std::uint16_t port_;
        MessageHandler messageHandler_;
        SOCKET socket_{INVALID_SOCKET};
        State state_{State::disconnected};

        std::chrono::steady_clock::time_point nextConnectionAttempt_;
        std::string incoming_;
        std::string outgoing_;
    };
}  // namespace sr2ap
