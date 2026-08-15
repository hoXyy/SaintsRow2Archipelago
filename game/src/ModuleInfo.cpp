#include "sr2ap/ModuleInfo.hpp"

#include "sr2ap/Memory.hpp"

#include <array>
#include <cstring>

namespace sr2ap {
    namespace {
        bool IsRangeInsideImage(std::uintptr_t base, std::size_t imageSize, std::uintptr_t address, std::size_t size) {
            if (address < base || size > imageSize) {
                return false;
            }
            const auto offset = address - base;
            return offset <= imageSize - size;
        }

        template <std::size_t Size>
        bool Matches(const ModuleInfo& module, std::uintptr_t rva, const char (&expected)[Size]) {
            if (!IsRangeInsideImage(module.base, module.imageSize, module.base + rva, Size)) {
                return false;
            }
            std::array<char, Size> actual{};
            return SafeCopy(reinterpret_cast<const void*>(module.base + rva), actual.data(), actual.size()) &&
                   std::memcmp(actual.data(), expected, Size) == 0;
        }
    }  // namespace

    std::optional<ModuleInfo> InspectModule(HMODULE module) {
        if (!module) {
            return std::nullopt;
        }
        const auto base = reinterpret_cast<std::uintptr_t>(module);
        IMAGE_DOS_HEADER dos{};
        if (!SafeCopy(reinterpret_cast<const void*>(base), &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew <= 0) {
            return std::nullopt;
        }
        const auto ntAddress = base + static_cast<std::uintptr_t>(dos.e_lfanew);
        IMAGE_NT_HEADERS nt{};
        if (ntAddress < base || !SafeCopy(reinterpret_cast<const void*>(ntAddress), &nt, sizeof(nt)) ||
            nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
            nt.OptionalHeader.SizeOfImage < sizeof(IMAGE_DOS_HEADER)) {
            return std::nullopt;
        }
        std::array<wchar_t, 32768> path{};
        const auto length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        ModuleInfo result;
        result.handle = module;
        result.base = base;
        result.imageSize = nt.OptionalHeader.SizeOfImage;
        result.peTimestamp = nt.FileHeader.TimeDateStamp;
        if (length && length < path.size()) {
            result.path.assign(path.data(), path.data() + length);
        }
        const auto sectionAddress =
            ntAddress + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + nt.FileHeader.SizeOfOptionalHeader;
        const auto sectionBytes =
            static_cast<std::size_t>(nt.FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
        if (!IsRangeInsideImage(base, result.imageSize, sectionAddress, sectionBytes)) {
            return std::nullopt;
        }
        for (unsigned index = 0; index < nt.FileHeader.NumberOfSections; ++index) {
            IMAGE_SECTION_HEADER section{};
            if (!SafeCopy(reinterpret_cast<const void*>(sectionAddress + index * sizeof(section)), &section,
                          sizeof(section))) {
                return std::nullopt;
            }
            char name[IMAGE_SIZEOF_SHORT_NAME + 1]{};
            std::memcpy(name, section.Name, IMAGE_SIZEOF_SHORT_NAME);
            if (!IsRangeInsideImage(base, result.imageSize, base + section.VirtualAddress, section.Misc.VirtualSize)) {
                continue;
            }
            result.sections.push_back(
                {name, base + section.VirtualAddress, section.Misc.VirtualSize, section.Characteristics});
        }
        return result;
    }

    std::optional<ModuleInfo> FindLoadedModule(const wchar_t* name) {
        return InspectModule(GetModuleHandleW(name));
    }

    std::optional<ModuleInfo> InspectSupportedGameModule() {
        auto game = InspectModule(GetModuleHandleW(nullptr));
        if (!game || !IsSupportedExecutable(*game)) {
            return std::nullopt;
        }
        return game;
    }

    bool IsSupportedExecutable(const ModuleInfo& module) {
        static constexpr char winMainPrologue[]{'\x55', '\x8B', '\xEC', '\x83'};
        static constexpr char patch12Pdb[]{
            "g:\\Projects\\SaintsRow\\sr2\\main\\code\\SR2___Win32_Final\\SR2_pc_final.pdb"};
        return Matches(module, 0x00120BA0, winMainPrologue) && Matches(module, 0x00A66958, patch12Pdb);
    }
}  // namespace sr2ap
