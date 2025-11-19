#ifndef VECTOR3_H
#define VECTOR3_H

#include "LegacyVector2.h"
#include "math_utils.h"

class LegacyVector3;

extern const LegacyVector3 g_upVector;
extern const LegacyVector3 g_zeroVector;

class LegacyVector3
{
  bool Compare(const LegacyVector3& b) const { return (NearlyEquals(x, b.x) && NearlyEquals(y, b.y) && NearlyEquals(z, b.z)); }

  public:
    float x, y, z;

    // *** Constructors ***
    LegacyVector3()
      : x(0.0f),
        y(0.0f),
        z(0.0f) {}

    LegacyVector3(float _x, float _y, float _z)
      : x(_x),
        y(_y),
        z(_z) {}

    LegacyVector3(const LegacyVector2& _b)
      : x(_b.x),
        y(0.0f),
        z(_b.y) {}

    LegacyVector3(XMVECTOR _b)
      : x(XMVectorGetX(_b)),
        y(XMVectorGetY(_b)),
        z(XMVectorGetZ(_b)) {}

    void Zero() { x = y = z = 0.0f; }

    void Set(float _x, float _y, float _z)
    {
      x = _x;
      y = _y;
      z = _z;
    }

    operator XMVECTOR() const { return XMVectorSet(x, y, z, 0.0f); }

    LegacyVector3 operator ^(const LegacyVector3& b) const
    {
      return LegacyVector3(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
    }

    float operator *(const LegacyVector3& b) const { return x * b.x + y * b.y + z * b.z; }

    LegacyVector3 operator -() const { return LegacyVector3(-x, -y, -z); }

    LegacyVector3 operator +(const LegacyVector3& b) const { return LegacyVector3(x + b.x, y + b.y, z + b.z); }

    LegacyVector3 operator -(const LegacyVector3& b) const { return LegacyVector3(x - b.x, y - b.y, z - b.z); }

    LegacyVector3 operator *(const float b) const { return LegacyVector3(x * b, y * b, z * b); }

    LegacyVector3 operator /(const float b) const
    {
      float multiplier = 1.0f / b;
      return LegacyVector3(x * multiplier, y * multiplier, z * multiplier);
    }

    const LegacyVector3& operator =(const LegacyVector3& b)
    {
      x = b.x;
      y = b.y;
      z = b.z;
      return *this;
    }

    const LegacyVector3& operator *=(const float b)
    {
      x *= b;
      y *= b;
      z *= b;
      return *this;
    }

    const LegacyVector3& operator /=(const float b)
    {
      float multiplier = 1.0f / b;
      x *= multiplier;
      y *= multiplier;
      z *= multiplier;
      return *this;
    }

    const LegacyVector3& operator +=(const LegacyVector3& b)
    {
      x += b.x;
      y += b.y;
      z += b.z;
      return *this;
    }

    const LegacyVector3& operator -=(const LegacyVector3& b)
    {
      x -= b.x;
      y -= b.y;
      z -= b.z;
      return *this;
    }

    const LegacyVector3& Normalize()
    {
      float lenSqrd = x * x + y * y + z * z;
      if (lenSqrd > 0.0f)
      {
        float invLen = 1.0f / sqrtf(lenSqrd);
        x *= invLen;
        y *= invLen;
        z *= invLen;
      }
      else
      {
        x = y = 0.0f;
        z = 1.0f;
      }

      return *this;
    }

    const LegacyVector3& SetLength(float _len)
    {
      float mag = Mag();
      if (NearlyEquals(mag, 0.0f))
      {
        x = _len;
        return *this;
      }

      float scaler = _len / Mag();
      *this *= scaler;
      return *this;
    }

    bool operator ==(const LegacyVector3& b) const;		// Uses FLT_EPSILON
    bool operator !=(const LegacyVector3& b) const;		// Uses FLT_EPSILON

    void RotateAroundX(float _angle);
    void RotateAroundY(float _angle);
    void RotateAroundZ(float _angle);
    void FastRotateAround(const LegacyVector3& _norm, float _angle);
    void RotateAround(const LegacyVector3& _norm);

    float Mag() const { return sqrtf(x * x + y * y + z * z); }

    float MagSquared() const { return x * x + y * y + z * z; }

    void HorizontalAndNormalize()
    {
      y = 0.0f;
      float invLength = 1.0f / sqrtf(x * x + z * z);
      x *= invLength;
      z *= invLength;
    }

    float* GetData() { return &x; }

    const float* GetDataConst() const { return &x; }
};

// Operator * between float and LegacyVector3
inline LegacyVector3 operator *(float _scale, const LegacyVector3& _v) { return _v * _scale; }

#endif
