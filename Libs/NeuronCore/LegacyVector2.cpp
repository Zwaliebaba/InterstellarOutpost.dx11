#include "pch.h"

#include "LegacyVector2.h"
#include "LegacyVector3.h"

#include <float.h>
#include <math.h>

// *******************
//  Private Functions
// *******************

// *** Compare
bool LegacyVector2::Compare(const LegacyVector2& b) const
{
  return ((fabsf(x - b.x) < FLT_EPSILON) && (fabsf(y - b.y) < FLT_EPSILON));
}

// ******************
//  Public Functions
// ******************

// Constructor
LegacyVector2::LegacyVector2()
  : x(0.0f),
    y(0.0f) {}

// Constructor
LegacyVector2::LegacyVector2(const LegacyVector3& v)
  : x(v.x),
    y(v.z) {}

// Constructor
LegacyVector2::LegacyVector2(float _x, float _y)
  : x(_x),
    y(_y) {}

void LegacyVector2::Zero() { x = y = 0.0f; }

void LegacyVector2::Set(float _x, float _y)
{
  x = _x;
  y = _y;
}

// Cross Product
float LegacyVector2::operator ^(const LegacyVector2& b) const { return x * b.y - y * b.x; }

// Dot Product
float LegacyVector2::operator *(const LegacyVector2& b) const { return (x * b.x + y * b.y); }

// Negate
LegacyVector2 LegacyVector2::operator -() const { return LegacyVector2(-x, -y); }

LegacyVector2 LegacyVector2::operator +(const LegacyVector2& b) const { return LegacyVector2(x + b.x, y + b.y); }

LegacyVector2 LegacyVector2::operator -(const LegacyVector2& b) const { return LegacyVector2(x - b.x, y - b.y); }

LegacyVector2 LegacyVector2::operator *(const float b) const { return LegacyVector2(x * b, y * b); }

LegacyVector2 LegacyVector2::operator /(const float b) const
{
  float multiplier = 1.0f / b;
  return LegacyVector2(x * multiplier, y * multiplier);
}

void LegacyVector2::operator =(const LegacyVector2& b)
{
  x = b.x;
  y = b.y;
}

// Assign from a LegacyVector3 - throws away Y value of LegacyVector3
void LegacyVector2::operator =(const LegacyVector3& b)
{
  x = b.x;
  y = b.z;
}

// Scale
void LegacyVector2::operator *=(const float b)
{
  x *= b;
  y *= b;
}

// Scale
void LegacyVector2::operator /=(const float b)
{
  float multiplier = 1.0f / b;
  x *= multiplier;
  y *= multiplier;
}

void LegacyVector2::operator +=(const LegacyVector2& b)
{
  x += b.x;
  y += b.y;
}

void LegacyVector2::operator -=(const LegacyVector2& b)
{
  x -= b.x;
  y -= b.y;
}

const LegacyVector2& LegacyVector2::Normalize()
{
  float lenSqrd = x * x + y * y;
  if (lenSqrd > 0.0f)
  {
    float invLen = 1.0f / sqrtf(lenSqrd);
    x *= invLen;
    y *= invLen;
  }
  else
  {
    x = 0.0f;
    y = 1.0f;
  }

  return *this;
}

void LegacyVector2::SetLength(float _len)
{
  float scaler = _len / Mag();
  x *= scaler;
  y *= scaler;
}

float LegacyVector2::Mag() const { return sqrtf(x * x + y * y); }

float LegacyVector2::MagSquared() const { return x * x + y * y; }

bool LegacyVector2::operator ==(const LegacyVector2& b) const { return Compare(b); }

bool LegacyVector2::operator !=(const LegacyVector2& b) const { return !Compare(b); }

float* LegacyVector2::GetData() { return &x; }
