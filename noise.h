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

inline uint32_t noise2d(const as::vec2i& position, uint32_t seed = 0)
{
  constexpr as::vec2i::value_type PrimeNumber = 198491317;
  return noise1d(position.x + PrimeNumber * position.y, seed);
}

inline float noise1dZeroToOne(int x_position, uint32_t seed = 0)
{
  return noise1d(x_position, seed)
       / static_cast<float>(std::numeric_limits<uint32_t>::max());
}

inline float noise2dZeroToOne(const as::vec2i& position, uint32_t seed = 0)
{
  return noise2d(position, seed)
       / static_cast<float>(std::numeric_limits<uint32_t>::max());
}

inline float noise1dMinusOneToOne(int x_position, uint32_t seed = 0)
{
  return (noise1dZeroToOne(x_position, seed) * 2.0f) - 1.0f;
}

inline float noise2dMinusOneToOne(const as::vec2i& position, uint32_t seed = 0)
{
  return (noise2dZeroToOne(position, seed) * 2.0f) - 1.0f;
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

as::vec2i vec2i_from_reals(const as::real x, const as::real y)
{
  return {as::vec2i::value_type(x), as::vec2i::value_type(y)};
}

as::vec2i vec2i_from_vec2(const as::vec2& v)
{
  return vec2i_from_reals(v.x, v.y);
}

as::vec2 gradient(as::real radians)
{
  return as::vec2{std::cos(radians), std::sin(radians)};
}

as::real angle(const as::vec2& position, uint32_t seed = 0)
{
  return noise2dZeroToOne(vec2i_from_vec2(position), seed) * 2.0f * as::k_pi;
}

inline float perlinNoise2d(const as::vec2& position, uint32_t seed = 0)
{
  const as::vec2 p0 = as::vec_floor(position);
  const as::vec2 p1 = p0 + as::vec2::axis_x();
  const as::vec2 p2 = p0 + as::vec2::axis_y();
  const as::vec2 p3 = p0 + as::vec2::one();

  const as::vec2 g0 = gradient(angle(p0, seed));
  const as::vec2 g1 = gradient(angle(p1, seed));
  const as::vec2 g2 = gradient(angle(p2, seed));
  const as::vec2 g3 = gradient(angle(p3, seed));

  const float t0 = position.x - p0.x;
  const float t1 = position.y - p0.y;

  const float p0p1 = as::mix(
    as::vec_dot(g0, position - p0), as::vec_dot(g1, position - p1),
    as::smoother_step(t0));
  const float p2p3 = as::mix(
    as::vec_dot(g2, position - p2), as::vec_dot(g3, position - p3),
    as::smoother_step(t0));

  return as::mix(p0p1, p2p3, as::smoother_step(t1));
}

} // namespace ns
