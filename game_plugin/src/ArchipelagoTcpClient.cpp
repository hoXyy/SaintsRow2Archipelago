#include "sr2ap/ArchipelagoTcpClient.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "rust/cxx.h"
#include "sr2ap/Logger.hpp"
#include "sr2ap/src/ffi/tcp_client_ffi.rs.h"

namespace sr2ap {
namespace {
::rust::Slice<const std::uint8_t> Bytes(const std::string_view value) noexcept {
    return {
        reinterpret_cast<const std::uint8_t*>(value.data()),
        value.size(),
    };
}
}  // namespace

class ArchipelagoTcpClient::Implementation {
   public:
    explicit Implementation(const std::uint16_t port)
        : client{rust::tcp_client_new(port)} {
    }

    ::rust::Box<rust::TcpClient> client;
};

ArchipelagoTcpClient::ArchipelagoTcpClient(const std::uint16_t port,
                                           MessageHandler messageHandler)
    : implementation_{std::make_unique<Implementation>(port)},
      messageHandler_{std::move(messageHandler)} {
}

ArchipelagoTcpClient::~ArchipelagoTcpClient() = default;

void ArchipelagoTcpClient::Poll() {
    auto result = rust::tcp_client_poll(*implementation_->client);

    connected_ = result.connected;

    for (const auto& line : result.lines) {
        const std::string_view message{
            line.data(),
            line.size(),
        };

        if (messageHandler_) {
            messageHandler_(message);
        }
    }
}

void ArchipelagoTcpClient::SendLine(const std::string_view message) {
    try {
        rust::tcp_client_send_line(*implementation_->client, Bytes(message));
    } catch (const ::rust::Error& error) {
        LogError("Network", std::string{"Could not queue outgoing message: "} +
                                error.what());
    }
}

bool ArchipelagoTcpClient::IsConnected() const noexcept {
    return connected_;
}
}  // namespace sr2ap
