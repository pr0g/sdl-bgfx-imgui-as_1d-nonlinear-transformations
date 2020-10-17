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

  auto mangled_bits = static_cast<uint32_t>(x_position);
  mangled_bits *= BitNoise1;
  mangled_bits += seed;
  mangled_bits ^= mangled_bits >> 8;
  mangled_bits += BitNoise2;
  mangled_bits ^= mangled_bits << 8;
  mangled_bits *= BitNoise3;
  mangled_bits ^= mangled_bits >> 8;

  return mangled_bits;
}

inline float noise1dZeroToOne(int x_position, uint32_t seed = 0)
{
  return noise1d(x_position, seed)
       / static_cast<float>(std::numeric_limits<uint32_t>::max());
}

inline float noise1dMinusOneToOne(int x_position, uint32_t seed = 0)
{
  return (noise1dZeroToOne(x_position, seed) * 2.0f) - 1.0f;
}

inline float perlinNoise1d(float x_position, uint32_t seed = 0)
{
  const float p0 = std::floor(x_position);
  const float p1 = p0 + 1.0f;
  const float t = x_position - p0;
  const float inv_t = -nlt::flip(t);
  const float g0 = noise1dMinusOneToOne(static_cast<int>(p0), seed);
  const float g1 = noise1dMinusOneToOne(static_cast<int>(p1), seed);
  return as::mix(g0 * t, g1 * inv_t, as::smoother_step(t));
}

} // namespace ns
