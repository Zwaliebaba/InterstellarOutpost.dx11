#ifndef MATRIX33_H
#define MATRIX33_H

#include "LegacyVector3.h"
#include "stdlib.h"	// For "NULL"

class Matrix33
{
  public:
    LegacyVector3 r, u, f;

    static float m_openGLFormat[16];

    // Constructors
    Matrix33();
    Matrix33(int _ignored);
    Matrix33(const Matrix33& _other);
    Matrix33(const LegacyVector3& _r, const LegacyVector3& _u, const LegacyVector3& _f);
    Matrix33(float _yaw, float _dive, float _roll);
    inline Matrix33(float _rx, float _ry, float _rz, float _ux, float _uy, float _uz, float _fx, float _fy, float _fz);

    void OrientRU(const LegacyVector3& _r, const LegacyVector3& _u);
    void OrientRF(const LegacyVector3& _r, const LegacyVector3& _f);
    void OrientUF(const LegacyVector3& _u, const LegacyVector3& _f);
    void OrientUR(const LegacyVector3& _u, const LegacyVector3& _r);
    void OrientFR(const LegacyVector3& _f, const LegacyVector3& _r);
    void OrientFU(const LegacyVector3& _f, const LegacyVector3& _u);
    const Matrix33& RotateAroundR(float angle);
    const Matrix33& RotateAroundU(float angle);
    const Matrix33& RotateAroundF(float angle);
    const Matrix33& RotateAroundX(float angle);
    const Matrix33& RotateAroundY(float angle);
    const Matrix33& RotateAroundZ(float angle);
    const Matrix33& RotateAround(const LegacyVector3& _rotationAxis);
    const Matrix33& RotateAround(const LegacyVector3& _norm, float _angle);
    const Matrix33& FastRotateAround(const LegacyVector3& _norm, float _angle);
    const Matrix33& Transpose();
    const Matrix33& Invert();
    LegacyVector3 DecomposeToYDR() const;	//x = dive, y = yaw, z = roll
    void DecomposeToYDR(float* _y, float* _d, float* _r) const;
    void SetToIdentity();
    void Normalize();

    LegacyVector3 InverseMultiplyVector(const LegacyVector3&) const;

    void OutputToDebugStream();
    float* ConvertToOpenGLFormat(const LegacyVector3* _pos = nullptr);

    // Operators
    const Matrix33& operator =(const Matrix33& _o);
    const Matrix33& operator *=(const Matrix33& _o);
    Matrix33 operator *(const Matrix33& b) const;
};

// Operator * between matrix33 and vector3
inline LegacyVector3 operator *(const Matrix33& _m, const LegacyVector3& _v)
{
  return LegacyVector3(_m.r.x * _v.x + _m.r.y * _v.y + _m.r.z * _v.z, _m.u.x * _v.x + _m.u.y * _v.y + _m.u.z * _v.z,
                       _m.f.x * _v.x + _m.f.y * _v.y + _m.f.z * _v.z);
}

// Operator * between vector3 and matrix33
inline LegacyVector3 operator *(const LegacyVector3& _v, const Matrix33& _m)
{
  return LegacyVector3(_m.r.x * _v.x + _m.r.y * _v.y + _m.r.z * _v.z, _m.u.x * _v.x + _m.u.y * _v.y + _m.u.z * _v.z,
                       _m.f.x * _v.x + _m.f.y * _v.y + _m.f.z * _v.z);
}

#endif
