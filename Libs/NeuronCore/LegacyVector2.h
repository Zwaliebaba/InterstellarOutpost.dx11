#ifndef INCLUDED_VECTOR2_H
#define INCLUDED_VECTOR2_H

class LegacyVector3;

class LegacyVector2
{
  bool Compare(const LegacyVector2& b) const;

  public:
    float x, y;

    LegacyVector2();
    LegacyVector2(const LegacyVector3&);
    LegacyVector2(float _x, float _y);

    void Zero();
    void Set(float _x, float _y);

    float operator ^(const LegacyVector2& b) const;	// Cross product
    float operator *(const LegacyVector2& b) const;	// Dot product

    operator XMVECTOR() const { return XMVectorSet(x, y, 0.0f, 0.0f); }

    LegacyVector2 operator -() const;					// Negate
    LegacyVector2 operator +(const LegacyVector2& b) const;
    LegacyVector2 operator -(const LegacyVector2& b) const;
    LegacyVector2 operator *(float b) const;		// Scale
    LegacyVector2 operator /(float b) const;

    void operator =(const LegacyVector2& b);
    void operator =(const LegacyVector3& b);
    void operator *=(float b);
    void operator /=(float b);
    void operator +=(const LegacyVector2& b);
    void operator -=(const LegacyVector2& b);

    bool operator ==(const LegacyVector2& b) const;		// Uses FLT_EPSILON
    bool operator !=(const LegacyVector2& b) const;		// Uses FLT_EPSILON

    const LegacyVector2& Normalize();
    void SetLength(float _len);

    float Mag() const;
    float MagSquared() const;

    float* GetData();
};

// Operator * between float and LegacyVector2
inline LegacyVector2 operator *(float _scale, const LegacyVector2& _v) { return _v * _scale; }

#endif
