#ifndef _INCLUDED_MATRIX34_H
#define _INCLUDED_MATRIX34_H

//#include <string.h>
#include <stdlib.h>

#include "LegacyVector3.h"
#include "matrix33.h"

class Matrix33;

class Matrix34
{
  public:
    LegacyVector3 r, u, f, pos;

    static float m_openGLFormat[16];

    // Constructors
    Matrix34() {}

    Matrix34([[maybe_unused]] int _ignored) { SetToIdentity(); }

    Matrix34(const Matrix34& _other) { memcpy(this, &_other, sizeof(Matrix34)); }

    Matrix34(const Matrix33& _or, const LegacyVector3& _pos)
      : r(_or.r),
        u(_or.u),
        f(_or.f),
        pos(_pos) {}

    Matrix34(const LegacyVector3& _f, const LegacyVector3& _u, const LegacyVector3& _pos)
      : f(_f),
        pos(_pos)
    {
      r = _u ^ f;
      r.Normalize();
      u = f ^ r;
      u.Normalize();
    }

    Matrix34(float _yaw, float _dive, float _roll)
    {
      SetToIdentity();
      RotateAroundZ(_roll);
      RotateAroundX(_dive);
      RotateAroundY(_yaw);
    }

    void OrientRU(const LegacyVector3& _r, const LegacyVector3& _u);
    void OrientRF(const LegacyVector3& _r, const LegacyVector3& _f);
    void OrientUF(const LegacyVector3& _u, const LegacyVector3& _f);
    void OrientUR(const LegacyVector3& _u, const LegacyVector3& _r);
    void OrientFR(const LegacyVector3& _f, const LegacyVector3& _r);
    void OrientFU(const LegacyVector3& _f, const LegacyVector3& _u);

    const Matrix34& RotateAroundR(float angle);
    const Matrix34& RotateAroundU(float angle);
    const Matrix34& RotateAroundF(float angle);
    const Matrix34& RotateAroundX(float angle);
    const Matrix34& RotateAroundY(float angle);
    const Matrix34& RotateAroundZ(float angle);
    const Matrix34& RotateAround(const LegacyVector3& _rotationAxis);
    const Matrix34& RotateAround(const LegacyVector3& _norm, float _angle);
    const Matrix34& FastRotateAround(const LegacyVector3& _norm, float _angle);

    const Matrix34& Transpose();

    LegacyVector3 DecomposeToYDR() const;	//x = dive, y = yaw, z = roll
    void DecomposeToYDR(float* _y, float* _d, float* _r) const;
    void SetToIdentity();

    bool IsNormalized();
    void Normalize();

    float* ConvertToOpenGLFormat() const
    {
      m_openGLFormat[0] = r.x;
      m_openGLFormat[1] = r.y;
      m_openGLFormat[2] = r.z;
      m_openGLFormat[3] = 0.0f;
      m_openGLFormat[4] = u.x;
      m_openGLFormat[5] = u.y;
      m_openGLFormat[6] = u.z;
      m_openGLFormat[7] = 0.0f;
      m_openGLFormat[8] = f.x;
      m_openGLFormat[9] = f.y;
      m_openGLFormat[10] = f.z;
      m_openGLFormat[11] = 0.0f;
      m_openGLFormat[12] = pos.x;
      m_openGLFormat[13] = pos.y;
      m_openGLFormat[14] = pos.z;
      m_openGLFormat[15] = 1.0f;

      return m_openGLFormat;
    }

    Matrix33 GetOr() const { return Matrix33(r, u, f); }

    LegacyVector3 InverseMultiplyVector(const LegacyVector3&) const;
    void WriteToDebugStream();

    void Test();

    // Operators
    bool operator ==(const Matrix34& b) const;		// Uses FLT_EPSILON
    //	Matrix34 const &operator =  ( Matrix34 const &_o );
    //	Matrix34 const &Matrix34::operator = ( Matrix34 const &_o )
    //	{
    //		memcpy(this, &_o, sizeof(Matrix34));
    //		return *this;
    //	}

    const Matrix34& operator *=(const Matrix34& _o);
    Matrix34 operator *(const Matrix34& b) const { return Matrix34(*this) *= b; }
};

extern const Matrix34 g_identityMatrix34;

// Operator * between matrix34 and vector3
inline LegacyVector3 operator *(const Matrix34& m, const LegacyVector3& v)
{
  return LegacyVector3(m.r.x * v.x + m.u.x * v.y + m.f.x * v.z + m.pos.x, m.r.y * v.x + m.u.y * v.y + m.f.y * v.z + m.pos.y,
                       m.r.z * v.x + m.u.z * v.y + m.f.z * v.z + m.pos.z);
}

// Operator * between vector3 and matrix34
inline LegacyVector3 operator *(const LegacyVector3& v, const Matrix34& m)
{
  return LegacyVector3(m.r.x * v.x + m.u.x * v.y + m.f.x * v.z + m.pos.x, m.r.y * v.x + m.u.y * v.y + m.f.y * v.z + m.pos.y,
                       m.r.z * v.x + m.u.z * v.y + m.f.z * v.z + m.pos.z);
}

#endif
