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

// returns binomial coefficient without explicit use of factorials,
// which can't be used with negative integers
float pascalTriangle(const float a, const int b)
{
  float result = 1.0f;
  for (int64_t i = 0; i < b; ++i) {
    result *= (a - i) / (i + 1);
  }
  return result;
}

// generalized smoothstep
float generalSmoothStep(int64_t power, float x)
{
  x = as::clamp(x, 0.0f, 1.0f); // x must be equal to or between 0 and 1
  float result = 0;
  for (int64_t n = 0; n <= power; ++n) {
    result += pascalTriangle(-power - 1.0f, n)
            * pascalTriangle(2.0f * power + 1.0f, power - n)
            * std::pow(x, power + n + 1);
  }
  return result;
}

} // namespace nlt
