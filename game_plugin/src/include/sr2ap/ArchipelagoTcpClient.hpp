#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace sr2ap {
class ArchipelagoTcpClient {
   public:
    using MessageHandler = std::function<void(std::string_view)>;

    explicit ArchipelagoTcpClient(std::uint16_t port,
                                  MessageHandler messageHandler = {});
    ~ArchipelagoTcpClient();

    ArchipelagoTcpClient(const ArchipelagoTcpClient&) = delete;
    ArchipelagoTcpClient& operator=(const ArchipelagoTcpClient&) = delete;
    ArchipelagoTcpClient(ArchipelagoTcpClient&&) = delete;
    ArchipelagoTcpClient& operator=(ArchipelagoTcpClient&&) = delete;

    void Poll();
    void SendLine(std::string_view message);

    [[nodiscard]] bool IsConnected() const noexcept;

   private:
    class Implementation;

    std::unique_ptr<Implementation> implementation_;
    MessageHandler messageHandler_;
    bool connected_{};
};
}  // namespace sr2ap
