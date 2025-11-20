#include "pch.h"

#include "texture_uv.h"


TextureUV::TextureUV() {}


TextureUV::TextureUV(float _u, float _v)
  : u(_u),
    v(_v) {}

// *** Set
void TextureUV::Set(float _u, float _v)
{
  u = _u;
  v = _v;
}

// *** Operator +
TextureUV TextureUV::operator +(const TextureUV& _b) const { return TextureUV(u + _b.u, v + _b.v); }

// *** Operator -
TextureUV TextureUV::operator -(const TextureUV& _b) const { return TextureUV(u - _b.u, v - _b.v); }

// *** Operator *
TextureUV TextureUV::operator *(const float _b) const { return TextureUV(u * _b, v * _b); }

// *** Operator /
TextureUV TextureUV::operator /(const float _b) const
{
  float multiplier = 1.0f / _b;
  return TextureUV(u * multiplier, v * multiplier);
}

// *** Operator =
const TextureUV& TextureUV::operator =(const TextureUV& _b)
{
  u = _b.u;
  v = _b.v;
  return *this;
}

// *** Operator *=
const TextureUV& TextureUV::operator *=(const float _b)
{
  u *= _b;
  v *= _b;
  return *this;
}

// *** Operator /=
const TextureUV& TextureUV::operator /=(const float _b)
{
  float multiplier = 1.0f / _b;
  u *= multiplier;
  v *= multiplier;
  return *this;
}

// *** Operator +=
const TextureUV& TextureUV::operator +=(const TextureUV& _b)
{
  u += _b.u;
  v += _b.v;
  return *this;
}

// *** Operator -=
const TextureUV& TextureUV::operator -=(const TextureUV& _b)
{
  u -= _b.u;
  v -= _b.v;
  return *this;
}

// *** GetData
float* TextureUV::GetData() { return &u; }
