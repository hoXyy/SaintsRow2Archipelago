#include <iomanip>
#include <sr2ap/Helpers.hpp>
#include <sstream>

namespace sr2ap {
std::string Hex(std::uintptr_t value, int width) {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(width)
           << std::setfill('0') << value;
    return stream.str();
}
}  // namespace sr2ap