#include "pch.h"

#include <math.h>
#include <memory.h>

#include "matrix34.h"
#include "matrix33.h"

float Matrix34::m_openGLFormat[16];

const Matrix34 g_identityMatrix34(0);

// ****************************************************************************
// Functions
// ****************************************************************************

void Matrix34::OrientRU(const LegacyVector3& _r, const LegacyVector3& _u)
{
  LegacyVector3 tr = _r;
  tr.Normalize();
  f = tr ^ _u;
  r = tr;
  f.Normalize();
  u = f ^ r;
}

void Matrix34::OrientRF(const LegacyVector3& _r, const LegacyVector3& _f)
{
  LegacyVector3 tr = _r;
  tr.Normalize();
  u = _f ^ tr;
  r = tr;
  u.Normalize();
  f = r ^ u;
}

void Matrix34::OrientUF(const LegacyVector3& _u, const LegacyVector3& _f)
{
  LegacyVector3 tu = _u;
  tu.Normalize();
  r = tu ^ _f;
  u = tu;
  r.Normalize();
  f = r ^ u;
}

void Matrix34::OrientUR(const LegacyVector3& _u, const LegacyVector3& _r)
{
  LegacyVector3 tu = _u;
  tu.Normalize();
  f = _r ^ tu;
  u = tu;
  f.Normalize();
  r = u ^ f;
}

void Matrix34::OrientFR(const LegacyVector3& _f, const LegacyVector3& _r)
{
  LegacyVector3 tf = _f;
  tf.Normalize();
  u = tf ^ _r;
  f = tf;
  u.Normalize();
  r = u ^ f;
}

void Matrix34::OrientFU(const LegacyVector3& _f, const LegacyVector3& _u)
{
  LegacyVector3 tf = _f;
  tf.Normalize();
  r = _u ^ tf;
  f = tf;
  r.Normalize();
  u = f ^ r;
}

const Matrix34& Matrix34::RotateAroundR(float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  float t;

  t = u.x * c + f.x * s;
  f.x = u.x * -s + f.x * c;
  u.x = t;

  t = u.y * c + f.y * s;
  f.y = u.y * -s + f.y * c;
  u.y = t;

  t = u.z * c + f.z * s;
  f.z = u.z * -s + f.z * c;
  u.z = t;

  return *this;
}

const Matrix34& Matrix34::RotateAroundU(float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  float t;

  t = f.x * c + r.x * s;
  r.x = f.x * -s + r.x * c;
  f.x = t;

  t = f.y * c + r.y * s;
  r.y = f.y * -s + r.y * c;
  f.y = t;

  t = f.z * c + r.z * s;
  r.z = f.z * -s + r.z * c;
  f.z = t;

  return *this;
}

const Matrix34& Matrix34::RotateAroundF(float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  float t;

  t = r.x * c + u.x * s;
  u.x = r.x * -s + u.x * c;
  r.x = t;

  t = r.y * c + u.y * s;
  u.y = r.y * -s + u.y * c;
  r.y = t;

  t = r.z * c + u.z * s;
  u.z = r.z * -s + u.z * c;
  r.z = t;

  return *this;
}

const Matrix34& Matrix34::RotateAroundX(float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  float t;

  t = r.y * c + r.z * s;
  r.z = r.y * -s + r.z * c;
  r.y = t;

  t = u.y * c + u.z * s;
  u.z = u.y * -s + u.z * c;
  u.y = t;

  t = f.y * c + f.z * s;
  f.z = f.y * -s + f.z * c;
  f.y = t;

  return *this;
}

const Matrix34& Matrix34::RotateAroundY(float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  float t;

  t = r.z * c + r.x * s;
  r.x = r.z * -s + r.x * c;
  r.z = t;

  t = u.z * c + u.x * s;
  u.x = u.z * -s + u.x * c;
  u.z = t;

  t = f.z * c + f.x * s;
  f.x = f.z * -s + f.x * c;
  f.z = t;

  return *this;
}

const Matrix34& Matrix34::RotateAroundZ(float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  float t;

  t = r.x * c + r.y * s;
  r.y = r.x * -s + r.y * c;
  r.x = t;

  t = u.x * c + u.y * s;
  u.y = u.x * -s + u.y * c;
  u.x = t;

  t = f.x * c + f.y * s;
  f.y = f.x * -s + f.y * c;
  f.x = t;

  return *this;
}

// Mag of vector determines the amount to rotate by. This function is useful
// when you want to do a rotation based on the result of a cross product
const Matrix34& Matrix34::RotateAround(const LegacyVector3& _rotationAxis)
{
  float magSquared = _rotationAxis.MagSquared();
  if (magSquared < 0.00001f * 0.00001f)
    return *this;

  float mag = sqrtf(magSquared);
  return FastRotateAround(_rotationAxis / mag, mag);
}

// Assumes normal is already normalised
const Matrix34& Matrix34::FastRotateAround(const LegacyVector3& _norm, float _angle)
{
  float s = sin(_angle);
  float c = cos(_angle);

  float dot = r * _norm;
  LegacyVector3 a = _norm * dot;
  LegacyVector3 n1 = r - a;
  LegacyVector3 n2 = _norm ^ n1;
  r = a + c * n1 + s * n2;

  dot = u * _norm;
  a = _norm * dot;
  n1 = u - a;
  n2 = _norm ^ n1;
  u = a + c * n1 + s * n2;

  dot = f * _norm;
  a = _norm * dot;
  n1 = f - a;
  n2 = _norm ^ n1;
  f = a + c * n1 + s * n2;

  return *this;
}

const Matrix34& Matrix34::RotateAround(const LegacyVector3& _onorm, float _angle)
{
  LegacyVector3 norm = _onorm;
  norm.Normalize();
  float s = sin(_angle);
  float c = cos(_angle);

  float dot = r * norm;
  LegacyVector3 a = norm * dot;
  LegacyVector3 n1 = r - a;
  LegacyVector3 n2 = norm ^ n1;
  r = a + c * n1 + s * n2;

  dot = u * norm;
  a = norm * dot;
  n1 = u - a;
  n2 = norm ^ n1;
  u = a + c * n1 + s * n2;

  dot = f * norm;
  a = norm * dot;
  n1 = f - a;
  n2 = norm ^ n1;
  f = a + c * n1 + s * n2;

  return *this;
}

const Matrix34& Matrix34::Transpose()
{
  pos = *this * -pos;

  float t = r.y;
  r.y = u.x;
  u.x = t;
  t = r.z;
  r.z = f.x;
  f.x = t;
  t = u.z;
  u.z = f.y;
  f.y = t;

  return *this;
}

LegacyVector3 Matrix34::DecomposeToYDR() const
{
  //x = dive, y = yaw, z = roll
  float x, y, z;
  DecomposeToYDR(&x, &y, &z);
  return LegacyVector3(x, y, z);
}

void Matrix34::DecomposeToYDR(float* _y, float* _d, float* _r) const
{
  //work with a copy..
  Matrix34 workingMat(*this);

  //get the yaw
  *_y = -atan2(workingMat.f.x, workingMat.f.z);

  //get the dive
  //	can't use asin( workingMat.f.y ) 'cos occasionally we get
  //	blasted precision problems. this sorts us out..
  *_d = atan2(workingMat.f.y, workingMat.u.y);

  //rotate the matrix back by -yaw and -dive (one at a time)
  workingMat.RotateAroundY(-*_y);
  workingMat.RotateAroundX(-*_d);

  //now the roll
  *_r = -atan2(workingMat.r.y, workingMat.r.x);
}

void Matrix34::SetToIdentity()
{
  memset(this, 0, sizeof(Matrix34));
  r.x = 1.0f;
  u.y = 1.0f;
  f.z = 1.0f;
}

bool Matrix34::IsNormalized()
{
  float lenR = r.MagSquared();
  if (lenR < 0.999f || lenR > 1.001f)
    return false;
  float lenU = u.MagSquared();
  if (lenU < 0.999f || lenU > 1.001f)
    return false;
  float lenF = f.MagSquared();
  if (lenF < 0.999f || lenF > 1.001f)
    return false;

  if (fabsf(r * u) > 0.001f)
    return false;
  if (fabsf(r * f) > 0.001f)
    return false;
  if (fabsf(f * u) > 0.001f)
    return false;

  return true;
}

void Matrix34::Normalize()
{
  f.Normalize();
  r = u ^ f;
  r.Normalize();
  u = f ^ r;
}

LegacyVector3 Matrix34::InverseMultiplyVector(const LegacyVector3& s) const
{
  LegacyVector3 v = s - pos;
  return LegacyVector3(r.x * v.x + u.x * v.y + f.x * v.z, r.y * v.x + u.y * v.y + f.y * v.z, r.z * v.x + u.z * v.y + f.z * v.z);
}

void Matrix34::WriteToDebugStream()
{
  DebugTrace("%5.2f %5.2f %5.2f\n", r.x, r.y, r.z);
  DebugTrace("%5.2f %5.2f %5.2f\n", u.x, u.y, u.z);
  DebugTrace("%5.2f %5.2f %5.2f\n", f.x, f.y, f.z);
  DebugTrace("%5.2f %5.2f %5.2f\n\n", pos.x, pos.y, pos.z);
}

void Matrix34::Test()
{
  Matrix34 a(LegacyVector3(0, 0, 1), g_upVector, g_zeroVector);
  Matrix34 b(LegacyVector3(0, 0, 1), g_upVector, g_zeroVector);
  Matrix34 c = a * b;
  DebugTrace("c = a * b\n");
  c.WriteToDebugStream();

  LegacyVector3 front(10, 20, 2);
  front.Normalize();
  LegacyVector3 up(0, 1, 0);
  LegacyVector3 right = up ^ front;
  right.Normalize();
  up = front ^ right;
  up.Normalize();
  Matrix34 d(front, up, LegacyVector3(-1, 2, -3));
  DebugTrace("d = \n");
  d.WriteToDebugStream();

  Matrix34 e = d * d;
  DebugTrace("e = d * d\n");
  e.WriteToDebugStream();
}

// ****************************************************************************
// Operators
// ****************************************************************************

bool Matrix34::operator ==(const Matrix34& _b) const
{
  if (r == _b.r && u == _b.u && f == _b.f && pos == _b.pos)
    return true;

  return false;
}

// Multiply this matrix by another and write the result back to ourself
const Matrix34& Matrix34::operator *=(const Matrix34& o)
{
  Matrix34 mat;

  mat.r.x = r.x * o.r.x + r.y * o.u.x + r.z * o.f.x;
  mat.r.y = r.x * o.r.y + r.y * o.u.y + r.z * o.f.y;
  mat.r.z = r.x * o.r.z + r.y * o.u.z + r.z * o.f.z;

  mat.u.x = u.x * o.r.x + u.y * o.u.x + u.z * o.f.x;
  mat.u.y = u.x * o.r.y + u.y * o.u.y + u.z * o.f.y;
  mat.u.z = u.x * o.r.z + u.y * o.u.z + u.z * o.f.z;

  mat.f.x = f.x * o.r.x + f.y * o.u.x + f.z * o.f.x;
  mat.f.y = f.x * o.r.y + f.y * o.u.y + f.z * o.f.y;
  mat.f.z = f.x * o.r.z + f.y * o.u.z + f.z * o.f.z;

  mat.pos.x = pos.x * o.r.x + pos.y * o.u.x + pos.z * o.f.x + o.pos.x;
  mat.pos.y = pos.x * o.r.y + pos.y * o.u.y + pos.z * o.f.y + o.pos.y;
  mat.pos.z = pos.x * o.r.z + pos.y * o.u.z + pos.z * o.f.z + o.pos.z;

  *this = mat;
  return *this;
}
