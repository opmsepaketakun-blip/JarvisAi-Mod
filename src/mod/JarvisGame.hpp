#pragma once

// Game access for Jarvis: signature scanning + one hook, done directly with the
// preloader (pl::memory), so no other mod has to be installed.
//
// The signature patterns and struct offsets below were copied from BedrockTools 1.5.12
// (https://github.com/QYCottage/BedrockTools) and target Minecraft Bedrock 1.26.6x.
// When the game updates they usually need refreshing from a newer BedrockTools release:
//   * kPatterns        <- src/core/memory/Signatures.cpp
//   * namespace offsets <- include/bedrocktools/sdk/offsets/*.hpp

#include <cstddef>
#include <cstdint>
#include <array>
#include <dlfcn.h>
#include <iterator>
#include <string>
#include <vector>

#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

namespace jsdk {

// ---------------------------------------------------------------------------
// Signatures (ARM64 patterns for Minecraft Bedrock 1.26.6x, taken from BedrockTools 1.5.12)
// ---------------------------------------------------------------------------
enum class Sig : std::size_t {
    ClientInstanceUpdate,
    ClientInstanceGetLocalPlayer,
    ActorFetchNearbyActorsSorted,
    ActorIsPlayer,
    LevelGetHitResult,
    HitResultGetEntity,
    BlockSourceGetBlock,
    ItemStackBaseGetRawNameId,
    ItemStackBaseGetDamageValue,
    Count
};

inline constexpr const char* kPatterns[static_cast<std::size_t>(Sig::Count)] = {
    "? ? ? FC ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 ? ? ? D1 59 D0 3B D5 ? ? ? 52 F5 03 02 2A",
    "? ? ? D1 ? ? ? A9 ? ? ? F9 ? ? ? 91 53 D0 3B D5 E8 03 00 AA ? ? ? 91 ? ? ? F9 ? ? ? 91 ? ? ? F8 ? ? ? ? ? ? ? 91 ? ? ? ? ? ? ? 36 ? ? ? 91 ? ? ? 52 ? ? ? ? ? ? ? ? E0 03 1F AA ? ? ? F9 ? ? ? F8 1F 01 09 EB ? ? ? 54 ? ? ? A9 ? ? ? F9 ? ? ? 91 C0 03 5F D6 ? ? ? ? C0 03 5F D6",
    "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 5B D0 3B D5 F3 03 08 AA F5 03 02 2A",
    "? ? ? F9 ? ? ? 52 ? ? ? 72 ? ? ? A9 08 01 0B CB ? ? ? D3 ? ? ? 51 0C 01 0A 8A ? ? ? B9 6B 0D 0C 8B ? ? ? F9 ? ? ? F9 ? ? ? B1 ? ? ? 54 8B 15 0B 8B ? ? ? B9 BF 01 0A 6B ? ? ? 54 ? ? ? F9 7F 01 09 EB ? ? ? 54 ? ? ? F9 ? ? ? B4 ? ? ? A9 ? ? ? D3 6B 01 09 CB 5F 0D 8B EB ? ? ? 54 ? ? ? F8 ? ? ? B4 ? ? ? 92 ? ? ? 12 ? ? ? 92 ? ? ? B8 28 01 08 4A ? ? ? 12 1F 01 09 6B E0 27 9F 1A C0 03 5F D6 E0 03 1F 2A C0 03 5F D6 ? ? ? D1 ? ? ? FD",
    "? ? ? F9 C0 03 5F D6 ? ? ? F9 ? ? ? ? ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 91 ? ? ? F9 ? ? ? F9 ? ? ? B4 ? ? ? A9 F3 03 00 AA F4 03 08 AA ? ? ? B4 ? ? ? 91 ? ? ? 52 ? ? ? ? ? ? ? F9 ? ? ? F9 ? ? ? B4 ? ? ? F9 ? ? ? 39 ? ? ? F9 ? ? ? 36 ? ? ? A9 ? ? ? A9 ? ? ? A8 C0 03 5F D6 ? ? ? ? ? ? ? 91",
    "? ? ? D1 ? ? ? A9 ? ? ? F9 ? ? ? 91 53 D0 3B D5 E8 03 00 AA ? ? ? 91 ? ? ? F9 ? ? ? 91 ? ? ? F8 ? ? ? ? ? ? ? 91 ? ? ? ? ? ? ? 36 ? ? ? 91 ? ? ? 52 ? ? ? ? ? ? ? ? E0 03 1F AA ? ? ? F9 ? ? ? F8 1F 01 09 EB ? ? ? 54 ? ? ? A9 ? ? ? F9 ? ? ? 91 C0 03 5F D6 ? ? ? ? ? ? ? 91",
    "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 56 D0 3B D5 ? ? ? F9 ? ? ? F9 ? ? ? B9 ? ? ? 79 1F 01 09 6B ? ? ? 54 ? ? ? 79 F3 03 00 AA 1F 01 09 6B ? ? ? 54 E0 03 00 91",
    "? ? ? A9 ? ? ? A9 FD 03 00 91 F3 03 08 AA ? ? ? F9 ? ? ? A9 ? ? ? F9 ? ? ? B4 ? ? ? F9",
    "? ? ? 79 C0 03 5F D6 08 24 21 8B ? ? ? 79 C0 03 5F D6 ? ? ? F9 ? ? ? B4 ? ? ? A9",
};

inline std::array<std::uintptr_t, static_cast<std::size_t>(Sig::Count)>& addresses() {
    static std::array<std::uintptr_t, static_cast<std::size_t>(Sig::Count)> table{};
    return table;
}

inline std::uintptr_t address(Sig id) { return addresses()[static_cast<std::size_t>(id)]; }

// Scans the library for every pattern in one pass. Returns true when ClientInstanceUpdate
// (the hook target) was found; the other entries may be 0 and simply disable one feature.
inline bool resolveAll(const char* library) {
    std::vector<std::string> patterns(std::begin(kPatterns), std::end(kPatterns));
    const auto resolved = pl::memory::resolveSignatures(patterns, library);
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        const auto it = resolved.find(patterns[i]);
        addresses()[i] = it != resolved.end() ? it->second : 0;
    }
    return address(Sig::ClientInstanceUpdate) != 0;
}

// ---------------------------------------------------------------------------
// Basic types and raw memory helpers
// ---------------------------------------------------------------------------
struct Vec2 { float x; float y; };
struct Vec3 { float x; float y; float z; };
struct BlockPos { int x; int y; int z; };

template <class T>
T& field(void* object, std::size_t offset) {
    return *reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(object) + offset);
}

template <class T>
const T& field(const void* object, std::size_t offset) {
    return *reinterpret_cast<const T*>(reinterpret_cast<std::uintptr_t>(object) + offset);
}

// ---------------------------------------------------------------------------
// Offsets for Minecraft Bedrock 1.26.6x (BedrockTools 1.5.12)
// ---------------------------------------------------------------------------
namespace offsets {
namespace Actor {
inline constexpr std::size_t mStateVectorComponent = 0x228;
inline constexpr std::size_t mActorRotationComponent = 0x238;
inline constexpr std::size_t mLevel = 0x1F0;
inline constexpr std::size_t mDimension = 0x1E0;
inline constexpr std::size_t mCategories = 0x220;
}
namespace Level { inline constexpr std::size_t mHitResultWrapper = 0x1D0; }
namespace HitResult {
inline constexpr std::size_t mStartPos = 0;
inline constexpr std::size_t mType = 24;
inline constexpr std::size_t mPos = 44;
}
namespace Dimension { inline constexpr std::size_t mBlockSource = 208; }
namespace Block { inline constexpr std::size_t mBlockType = 0x68; }
namespace BlockType { inline constexpr std::size_t mNameInfo = 0x88; }
namespace NameInfo { inline constexpr std::size_t mFullName = 0x40; }
namespace HashedString { inline constexpr std::size_t mString = 0x8; }
namespace VTable {
inline constexpr std::size_t PlayerGetCarriedItem = 78;
inline constexpr std::size_t ClientInstanceGetLocalPlayer = 31;
inline constexpr std::size_t ItemGetMaxDamage = 37;
}
namespace ItemStack {
inline constexpr std::size_t ItemStackBaseItem = 0x8;
inline constexpr std::size_t SharedCounterPointer = 0x0;
}
}

// ---------------------------------------------------------------------------
// Thin wrappers over game objects (no virtuals, they only read memory)
// ---------------------------------------------------------------------------
class Actor {
public:
    Vec3 position() const {
        void* component = field<void*>(this, offsets::Actor::mStateVectorComponent);
        return component ? field<Vec3>(component, 0) : Vec3{};
    }
    Vec2 rotation() const {   // x = pitch, y = yaw
        void* component = field<void*>(this, offsets::Actor::mActorRotationComponent);
        return component ? field<Vec2>(component, 0) : Vec2{};
    }
    std::uint32_t categories() const { return field<std::uint32_t>(this, offsets::Actor::mCategories); }
    void* level() const { return field<void*>(this, offsets::Actor::mLevel); }
    void* dimension() const { return field<void*>(this, offsets::Actor::mDimension); }
};

class Player : public Actor {};

// Level -> stored hit result (same object LevelGetHitResult returns)
inline void* storedHitResult(void* level) {
    return level ? field<void*>(level, offsets::Level::mHitResultWrapper) : nullptr;
}

inline void* blockSourceOf(void* dimension) {
    return dimension ? field<void*>(dimension, offsets::Dimension::mBlockSource) : nullptr;
}

// Returns nullptr when the block / block type is missing.
inline const std::string* blockFullName(void* block) {
    if (!block) return nullptr;
    auto type = reinterpret_cast<std::uintptr_t>(field<void*>(block, offsets::Block::mBlockType));
    if (!type) return nullptr;
    auto hashed = type + offsets::BlockType::mNameInfo + offsets::NameInfo::mFullName;
    return reinterpret_cast<const std::string*>(hashed + offsets::HashedString::mString);
}

}  // namespace jsdk
