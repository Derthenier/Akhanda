module;

#include <string>
#include <string_view>
#include <cstdint>

export module Akhanda.Core.Hash;

export namespace Akhanda::Core {
    using Hash64 = std::uint64_t;
    using Hash32 = std::uint32_t;

    constexpr Hash64 FNV_OFFSET_BASIS_64 = 14695981039346656037ULL;
    constexpr Hash64 FNV_PRIME_64 = 1099511628211ULL;
    
    constexpr Hash32 FNV_OFFSET_BASIS_32 = 2166136261U;
    constexpr Hash32 FNV_PRIME_32 = 16777619U;

    constexpr Hash64 HashString64(std::string_view str) noexcept {
        Hash64 hash = FNV_OFFSET_BASIS_64;
        for (char c : str) {
            hash ^= static_cast<Hash64>(static_cast<unsigned char>(c));
            hash *= FNV_PRIME_64;
        }
        return hash;
    }

    constexpr Hash32 HashString32(std::string_view str) noexcept {
        Hash32 hash = FNV_OFFSET_BASIS_32;
        for (char c : str) {
            hash ^= static_cast<Hash32>(static_cast<unsigned char>(c));
            hash *= FNV_PRIME_32;
        }
        return hash;
    }

    inline Hash64 HashString(const std::string& str) noexcept {
        return HashString64(str);
    }

    inline Hash64 HashString(std::string_view str) noexcept {
        return HashString64(str);
    }

    inline Hash64 HashString(const char* str) noexcept {
        return HashString64(std::string_view(str));
    }
}