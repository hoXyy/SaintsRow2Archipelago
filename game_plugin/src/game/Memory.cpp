#include "Memory.hpp"

#include <Zydis/Zydis.h>

#include <array>
#include <limits>

#include "ModuleInfo.hpp"

namespace sr2ap {
namespace {
constexpr std::size_t kDecodeBufferSize{ZYDIS_MAX_INSTRUCTION_LENGTH + 1};

bool HasAccess(DWORD protect, bool executable) {
    if ((protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) {
        return false;
    }
    const auto basic = protect & 0xFF;
    if (executable) {
        return basic == PAGE_EXECUTE || basic == PAGE_EXECUTE_READ ||
               basic == PAGE_EXECUTE_READWRITE ||
               basic == PAGE_EXECUTE_WRITECOPY;
    }
    return basic == PAGE_READONLY || basic == PAGE_READWRITE ||
           basic == PAGE_WRITECOPY || basic == PAGE_EXECUTE_READ ||
           basic == PAGE_EXECUTE_READWRITE || basic == PAGE_EXECUTE_WRITECOPY;
}

bool DecodeInstruction(const std::uint8_t* bytes, std::size_t size,
                       ZydisDecodedInstruction& instruction,
                       ZydisDecodedOperand (&operands)
                           [ZYDIS_MAX_OPERAND_COUNT]) {
    ZydisDecoder decoder{};
    return ZYAN_SUCCESS(ZydisDecoderInit(
               &decoder, ZYDIS_MACHINE_MODE_LEGACY_32,
               ZYDIS_STACK_WIDTH_32)) &&
           ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bytes, size,
                                               &instruction, operands));
}

const ZydisDecodedOperand* FirstVisibleOperand(
    const ZydisDecodedInstruction& instruction,
    const ZydisDecodedOperand (&operands)[ZYDIS_MAX_OPERAND_COUNT]) {
    return instruction.operand_count_visible == 0 ? nullptr : &operands[0];
}
}  // namespace

bool IsReadableAddress(const void* address, std::size_t size) {
    if (!address || size == 0) {
        return false;
    }
    auto cursor = reinterpret_cast<std::uintptr_t>(address);
    const auto end = cursor + size;
    if (end < cursor) {
        return false;
    }
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<const void*>(cursor), &info,
                          sizeof(info)) ||
            info.State != MEM_COMMIT || !HasAccess(info.Protect, false)) {
            return false;
        }
        const auto regionEnd =
            reinterpret_cast<std::uintptr_t>(info.BaseAddress) +
            info.RegionSize;
        if (regionEnd <= cursor) {
            return false;
        }
        cursor = regionEnd < end ? regionEnd : end;
    }
    return true;
}

bool IsExecutableAddress(const void* address) {
    MEMORY_BASIC_INFORMATION info{};
    return address && VirtualQuery(address, &info, sizeof(info)) &&
           info.State == MEM_COMMIT && HasAccess(info.Protect, true);
}

bool IsInsideModule(HMODULE module, const void* address) {
    const auto info = InspectModule(module);

    if (!info || !address) {
        return false;
    }

    const auto value = reinterpret_cast<std::uintptr_t>(address);
    return value >= info->base && value < info->base + info->imageSize;
}

bool SafeCopy(const void* address, void* destination, std::size_t size) {
    if (!destination || !IsReadableAddress(address, size)) {
        return false;
    }

    SIZE_T copied{};
    return ReadProcessMemory(GetCurrentProcess(), address, destination, size,
                             &copied) &&
           copied == size;
}

MemoryWriteResult WriteExecutableMemory(void* destination,
                                        std::span<const std::uint8_t> bytes) {
    if (!destination || bytes.empty()) {
        return MemoryWriteResult{.bytesWritten = false,
                                 .cacheFlushed = false,
                                 .protectionRestored = false};
    }

    DWORD previousProtection{};
    if (!VirtualProtect(destination, bytes.size(), PAGE_EXECUTE_READWRITE,
                        &previousProtection)) {
        return MemoryWriteResult{.bytesWritten = false,
                                 .cacheFlushed = false,
                                 .protectionRestored = false};
    }

    SIZE_T written{};
    const bool bytesWritten =
        WriteProcessMemory(GetCurrentProcess(), destination, bytes.data(),
                           bytes.size(), &written) != FALSE &&
        written == bytes.size();

    const bool flushSucceeded =
        bytesWritten && FlushInstructionCache(GetCurrentProcess(), destination,
                                              bytes.size()) != FALSE;

    DWORD ignoredProtection{};
    const bool restoreSucceeded =
        VirtualProtect(destination, bytes.size(), previousProtection,
                       &ignoredProtection) != FALSE;

    return MemoryWriteResult{.bytesWritten = bytesWritten,
                             .cacheFlushed = flushSucceeded,
                             .protectionRestored = restoreSucceeded};
}

std::optional<std::string> ReadFixedString(std::uintptr_t address,
                                           std::size_t capacity) {
    std::string buffer(capacity, '\0');
    if (!SafeCopy(reinterpret_cast<const void*>(address), buffer.data(),
                  buffer.size())) {
        return std::nullopt;
    }
    const auto terminator = buffer.find('\0');
    if (terminator == std::string::npos) {
        return std::nullopt;
    }
    buffer.resize(terminator);
    return buffer;
}

std::optional<std::vector<std::uint8_t>> CaptureBytes(const void* address,
                                                      std::size_t size) {
    std::vector<std::uint8_t> result(size);

    if (!SafeCopy(address, result.data(), size)) {
        return std::nullopt;
    }

    return result;
}

std::optional<std::uintptr_t> ResolveRelativeCallTarget(
    std::uintptr_t address) {
    std::array<std::uint8_t, ZYDIS_MAX_INSTRUCTION_LENGTH> bytes{};
    if (!SafeCopy(reinterpret_cast<const void*>(address), bytes.data(),
                  bytes.size())) {
        return std::nullopt;
    }

    ZydisDecodedInstruction instruction{};
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
    if (!DecodeInstruction(bytes.data(), bytes.size(), instruction, operands) ||
        instruction.mnemonic != ZYDIS_MNEMONIC_CALL) {
        return std::nullopt;
    }

    const auto* const operand = FirstVisibleOperand(instruction, operands);
    if (!operand || operand->type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
        !operand->imm.is_relative) {
        return std::nullopt;
    }

    ZyanU64 target{};
    if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(
            &instruction, operand, static_cast<ZyanU64>(address), &target)) ||
        target > std::numeric_limits<std::uintptr_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uintptr_t>(target);
}

DetourKind DetectDetour(const void* address) {
    std::array<std::uint8_t, kDecodeBufferSize> bytes{};
    if (!SafeCopy(address, bytes.data(), bytes.size())) {
        return DetourKind::Unknown;
    }

    ZydisDecodedInstruction instruction{};
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
    if (!DecodeInstruction(bytes.data(), bytes.size(), instruction, operands)) {
        return DetourKind::Unknown;
    }

    const auto* const operand = FirstVisibleOperand(instruction, operands);
    if (instruction.mnemonic == ZYDIS_MNEMONIC_CALL && operand &&
        operand->type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
        operand->imm.is_relative) {
        return DetourKind::RelativeCall;
    }

    if (instruction.mnemonic == ZYDIS_MNEMONIC_JMP && operand) {
        if (operand->type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
            operand->imm.is_relative) {
            return DetourKind::RelativeJump;
        }
        return DetourKind::IndirectJump;
    }

    if (instruction.mnemonic == ZYDIS_MNEMONIC_PUSH && operand &&
        operand->type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
        instruction.length < bytes.size()) {
        ZydisDecodedInstruction next{};
        ZydisDecodedOperand nextOperands[ZYDIS_MAX_OPERAND_COUNT]{};
        if (DecodeInstruction(bytes.data() + instruction.length,
                              bytes.size() - instruction.length, next,
                              nextOperands) &&
            next.mnemonic == ZYDIS_MNEMONIC_RET) {
            return DetourKind::PushReturn;
        }
    }

    return DetourKind::None;
}

const char* ToString(DetourKind kind) {
    switch (kind) {
        case DetourKind::None:
            return "none";
        case DetourKind::RelativeJump:
            return "relative_jump";
        case DetourKind::RelativeCall:
            return "relative_call";
        case DetourKind::IndirectJump:
            return "indirect_jump";
        case DetourKind::PushReturn:
            return "push_return";
        case DetourKind::Unknown:
            return "unknown";
    }

    return "unknown";
}
}  // namespace sr2ap
