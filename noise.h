#pragma once

#include <cstdint>
#include <numeric>

namespace ns
{

// SquirrelNoise (SquirrelNoise3 by Squirrel Eiserloh)
// https://www.gdcvault.com/play/1024365/Math-for-Game-Programmers-Noise
inline uint32_t noise1d(int x_position, uint32_t seed = 0)
{
    constexpr uint32_t BitNoise1 = 0x68e31d4a;
    constexpr uint32_t BitNoise2 = 0xB5297a4d;
    constexpr uint32_t BitNoise3 = 0x1b56c4e9;

    uint32_t mangledBits = static_cast<uint32_t>(x_position);
    mangledBits *= BitNoise1;
    mangledBits += seed;
    mangledBits ^= mangledBits >> 8;
    mangledBits += BitNoise2;
    mangledBits ^= mangledBits << 8;
    mangledBits *= BitNoise3;
    mangledBits ^= mangledBits >> 8;

    return mangledBits;
}

inline float noise1dZeroToOne(int x_position, uint32_t seed = 0)
{
    return noise1d(x_position, seed) / static_cast<float>(std::numeric_limits<uint32_t>::max());
}

inline float noise1dMinusOneToOne(int x_position, uint32_t seed = 0)
{
    return (noise1dZeroToOne(x_position, seed) * 2.0f) - 1.0f;
}

} // namespace ns
