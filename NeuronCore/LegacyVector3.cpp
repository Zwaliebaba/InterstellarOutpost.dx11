#include "pch.h"

#include <float.h>
#include <math.h>

#include "LegacyVector3.h"
#include "LegacyVector2.h"
#include "math_utils.h"

const LegacyVector3 g_upVector(0.0f, 1.0f, 0.0f);
const LegacyVector3 g_zeroVector(0.0f, 0.0f, 0.0f);

// ******************
//  Public Functions
// ******************

void LegacyVector3::RotateAroundX(float _angle) { FastRotateAround(LegacyVector3(1, 0, 0), _angle); }

void LegacyVector3::RotateAroundY(float _angle) { FastRotateAround(g_upVector, _angle); }

void LegacyVector3::RotateAroundZ(float _angle) { FastRotateAround(LegacyVector3(0, 0, 1), _angle); }

// *** FastRotateAround
// ASSUMES that _norm is normalised
void LegacyVector3::FastRotateAround(const LegacyVector3& _norm, float _angle)
{
  float dot = (*this) * _norm;
  LegacyVector3 a = _norm * dot;
  LegacyVector3 n1 = *this - a;
  LegacyVector3 n2 = _norm ^ n1;
  float s = sin(_angle);
  float c = cos(_angle);

  *this = a + c * n1 + s * n2;
}

void LegacyVector3::RotateAround(const LegacyVector3& _axis)
{
  float angle = _axis.MagSquared();
  if (angle < 1e-8)
    return;
  angle = sqrtf(angle);
  LegacyVector3 norm(_axis / angle);
  FastRotateAround(norm, angle);
}

// *** Operator ==
bool LegacyVector3::operator ==(const LegacyVector3& b) const { return Compare(b); }

// *** Operator !=
bool LegacyVector3::operator !=(const LegacyVector3& b) const { return !Compare(b); }
