// Copyright (c) 2014, Jan Winkler <winkler@cs.uni-bremen.de>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Universität Bremen nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/* Author: Jan Winkler */

#include <cmath>

/*! \brief Convert RGB to HSV color space

  Converts a given set of RGB values `r', `g', `b' into HSV
  coordinates. The input RGB values are in the range [0, 1], and the
  output HSV values are in the ranges h = [0, 360], and s, v = [0,
  1], respectively.

  \param fR Red component, used as input, range: [0, 1]
  \param fG Green component, used as input, range: [0, 1]
  \param fB Blue component, used as input, range: [0, 1]
  \param fH Hue component, used as output, range: [0, 360]
  \param fS Hue component, used as output, range: [0, 1]
  \param fV Hue component, used as output, range: [0, 1]

*/
inline void RGBtoHSV(
  const float fR, const float fG, const float fB, float& fH, float& fS,
  float& fV)
{
  float fCMax = std::max(std::max(fR, fG), fB);
  float fCMin = std::min(std::min(fR, fG), fB);
  float fDelta = fCMax - fCMin;

  if (fDelta > 0.0f) {
    if (fCMax == fR) {
      fH = 60.0f * (fmodf(((fG - fB) / fDelta), 6.0f));
    } else if (fCMax == fG) {
      fH = 60.0f * (((fB - fR) / fDelta) + 2.0f);
    } else if (fCMax == fB) {
      fH = 60.0f * (((fR - fG) / fDelta) + 4.0f);
    }

    if (fCMax > 0.0f) {
      fS = fDelta / fCMax;
    } else {
      fS = 0.0f;
    }

    fV = fCMax;
  } else {
    fH = 0.0f;
    fS = 0.0f;
    fV = fCMax;
  }

  if (fH < 0.0f) {
    fH = 360.0f + fH;
  }
}

/*! \brief Convert HSV to RGB color space

  Converts a given set of HSV values `h', `s', `v' into RGB
  coordinates. The output RGB values are in the range [0, 1], and
  the input HSV values are in the ranges h = [0, 360], and s, v =
  [0, 1], respectively.

  \param fR Red component, used as output, range: [0, 1]
  \param fG Green component, used as output, range: [0, 1]
  \param fB Blue component, used as output, range: [0, 1]
  \param fH Hue component, used as input, range: [0, 360]
  \param fS Hue component, used as input, range: [0, 1]
  \param fV Hue component, used as input, range: [0, 1]

*/
inline void HSVtoRGB(
  float& fR, float& fG, float& fB, const float fH, const float fS,
  const float fV)
{
  float fC = fV * fS; // Chroma
  float fHPrime = fmodf(fH / 60.0f, 6.0f);
  float fX = fC * (1 - fabsf(fmodf(fHPrime, 2.0f) - 1.0f));
  float fM = fV - fC;

  if (0.0f <= fHPrime && fHPrime < 1.0f) {
    fR = fC;
    fG = fX;
    fB = 0.0f;
  } else if (1.0f <= fHPrime && fHPrime < 2.0f) {
    fR = fX;
    fG = fC;
    fB = 0.0f;
  } else if (2.0f <= fHPrime && fHPrime < 3.0f) {
    fR = 0.0f;
    fG = fC;
    fB = fX;
  } else if (3.0f <= fHPrime && fHPrime < 4.0f) {
    fR = 0.0f;
    fG = fX;
    fB = fC;
  } else if (4.0f <= fHPrime && fHPrime < 5.0f) {
    fR = fX;
    fG = 0.0f;
    fB = fC;
  } else if (5.0f <= fHPrime && fHPrime < 6.0f) {
    fR = fC;
    fG = 0.0f;
    fB = fX;
  } else {
    fR = 0.0f;
    fG = 0.0f;
    fB = 0.0f;
  }

  fR += fM;
  fG += fM;
  fB += fM;
}
