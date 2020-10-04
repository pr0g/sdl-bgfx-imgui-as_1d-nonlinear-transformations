#pragma once

#include "as/as-math-ops.hpp"

namespace nlt
{

inline float flip(const float x)
{
  return 1.0f - as::clamp(x, 0.0f, 1.0f);
}

inline float smoothStart2(const float x)
{
  return x * x;
}

inline float smoothStart3(const float x)
{
  return x * x * x;
}

inline float smoothStart4(const float x)
{
  return x * x * x * x;
}

inline float smoothStart5(const float x)
{
  return x * x * x * x * x;
}

inline float smoothStop2(const float x)
{
  return flip(flip(x) * flip(x));
}

inline float smoothStop3(const float x)
{
  return flip(flip(x) * flip(x) * flip(x));
}

inline float smoothStop4(const float x)
{
  return flip(flip(x) * flip(x) * flip(x) * flip(x));
}

inline float smoothStop5(const float x)
{
  return flip(flip(x) * flip(x) * flip(x) * flip(x) * flip(x));
}

inline float bezierSmoothStep(const float x)
{
  return (3.0f * x * x) - (2.0f * x * x * x);
}

inline float normalizedBezier2(const float b, const float t)
{
  const float s = 1.0f - t;
  const float t2 = t * t;
  const float st = t * s;
  return 2.0f * st * b + t2;
}

inline float normalizedBezier3(const float b, const float c, const float t)
{
  const float s = 1.0f - t;
  const float t2 = t * t;
  const float t3 = t2 * t;
  const float s2 = s * s;
  return (3.0f * b * s2 * t) + (3.0f * c * s * t2) + t3;
}

inline float normalizedBezier4(
  const float b, const float c, const float d, const float t)
{
  const float s = 1.0f - t;
  const float t2 = t * t;
  const float t3 = t2 * t;
  const float t4 = t3 * t;
  const float s2 = s * s;
  const float s3 = s2 * s;
  return (4.0f * b * s3 * t) + (6.0f * c * s2 * t2) + (4.0f * s * t3 * d) + t4;
}

inline float normalizedBezier5(
  const float b, const float c, const float d, const float e, const float t)
{
  const float s = 1.0f - t;
  const float t2 = t * t;
  const float t3 = t2 * t;
  const float t4 = t3 * t;
  const float t5 = t4 * t;
  const float s2 = s * s;
  const float s3 = s2 * s;
  const float s4 = s3 * s;
  const float s5 = s4 * s;

  return (5.0f * s4 * t * b) + (10.0f * s3 * t2 * c) + (10.0f * s2 * t3 * d)
       + (5.0f * s * t4 * e) + t5;
}

inline float smoothStepMixer(const float t1, const float t2)
{
  return as::mix(smoothStart2(t1), smoothStop2(t1), t2);
}

inline float smoothStepMixed(const float t)
{
  return smoothStepMixer(t, t);
}

inline float smootherStepMixer(const float t1, const float t2)
{
  return as::mix(smoothStart3(t1), smoothStop3(t1), smoothStepMixed(t2));
}

inline float smootherStepMixed(const float t)
{
  return smootherStepMixer(t, t);
}

inline as::vec3 bezier1(const as::vec3& p0, const as::vec3& p1, const float t)
{
  return as::vec_mix(p0, p1, t);
}

inline as::vec3 bezier2(
  const as::vec3& p0, const as::vec3& p1, const as::vec3& c0, const float t)
{
  const as::vec3 p0c0 = as::vec_mix(p0, c0, t);
  const as::vec3 c0p1 = as::vec_mix(c0, p1, t);
  return as::vec_mix(p0c0, c0p1, t);
}

inline as::vec3 bezier3(
  const as::vec3& p0, const as::vec3& p1, const as::vec3& c0,
  const as::vec3& c1, const float t)
{
  const as::vec3 p0c0 = as::vec_mix(p0, c0, t);
  const as::vec3 c0c1 = as::vec_mix(c0, c1, t);
  const as::vec3 c1p1 = as::vec_mix(c1, p1, t);
  const as::vec3 p0c0_c0c1 = as::vec_mix(p0c0, c0c1, t);
  const as::vec3 c0c1_c1p1 = as::vec_mix(c0c1, c1p1, t);

  return as::vec_mix(p0c0_c0c1, c0c1_c1p1, t);
}

inline as::vec3 bezier4(
  const as::vec3& p0, const as::vec3& p1, const as::vec3& c0,
  const as::vec3& c1, const as::vec3& c2, const float t)
{
  const as::vec3 p0c0 = as::vec_mix(p0, c0, t);
  const as::vec3 c0c1 = as::vec_mix(c0, c1, t);
  const as::vec3 c1c2 = as::vec_mix(c1, c2, t);
  const as::vec3 c2p1 = as::vec_mix(c2, p1, t);
  const as::vec3 p0c0_c0c1 = as::vec_mix(p0c0, c0c1, t);
  const as::vec3 c0c1_c1c2 = as::vec_mix(c0c1, c1c2, t);
  const as::vec3 c1c2_c2p1 = as::vec_mix(c1c2, c2p1, t);
  const as::vec3 p0c0c0c1_c0c1c1c2 = as::vec_mix(p0c0_c0c1, c0c1_c1c2, t);
  const as::vec3 c0c1c1c2_c1c2c2p1 = as::vec_mix(c0c1_c1c2, c1c2_c2p1, t);

  return as::vec_mix(p0c0c0c1_c0c1c1c2, c0c1c1c2_c1c2c2p1, t);
}

inline as::vec3 bezier5(
  const as::vec3& p0, const as::vec3& p1, const as::vec3& c0,
  const as::vec3& c1, const as::vec3& c2, const as::vec3& c3, const float t)
{
  const as::vec3 p0c0 = as::vec_mix(p0, c0, t);
  const as::vec3 c0c1 = as::vec_mix(c0, c1, t);
  const as::vec3 c1c2 = as::vec_mix(c1, c2, t);
  const as::vec3 c2c3 = as::vec_mix(c2, c3, t);
  const as::vec3 c3p1 = as::vec_mix(c3, p1, t);
  const as::vec3 p0c0_c0c1 = as::vec_mix(p0c0, c0c1, t);
  const as::vec3 c0c1_c1c2 = as::vec_mix(c0c1, c1c2, t);
  const as::vec3 c1c2_c2c3 = as::vec_mix(c1c2, c2c3, t);
  const as::vec3 c2c3_c3p1 = as::vec_mix(c2c3, c3p1, t);
  const as::vec3 p0c0c0c1_c0c1c1c2 = as::vec_mix(p0c0_c0c1, c0c1_c1c2, t);
  const as::vec3 c0c1c1c2_c1c2c2c3 = as::vec_mix(c0c1_c1c2, c1c2_c2c3, t);
  const as::vec3 c1c2c2c3_c2c3c3p1 = as::vec_mix(c1c2_c2c3, c2c3_c3p1, t);
  const as::vec3 p0c0c0c1c0c1c1c2_c0c1c1c2c1c2c2c3 =
    as::vec_mix(p0c0c0c1_c0c1c1c2, c0c1c1c2_c1c2c2c3, t);
  const as::vec3 c0c1c1c2c1c2c2c3_c1c2c2c3c2c3c3p1 =
    as::vec_mix(c0c1c1c2_c1c2c2c3, c1c2c2c3_c2c3c3p1, t);

  return as::vec_mix(
    p0c0c0c1c0c1c1c2_c0c1c1c2c1c2c2c3, c0c1c1c2c1c2c2c3_c1c2c2c3c2c3c3p1, t);
}

} // namespace nlt
