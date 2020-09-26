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

inline float bezierSmootherStep(const float x)
{
  return (3 * x * x) - (2 * x * x * x);
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
