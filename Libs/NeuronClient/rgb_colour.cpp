#include "pch.h"
#include "rgb_colour.h"

RGBAColor g_colourBlack(0, 0, 0);
RGBAColor g_colourWhite(255, 255, 255);


RGBAColor::RGBAColor() {}


RGBAColor::RGBAColor(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) { Set(_r, _g, _b, _a); }

RGBAColor::RGBAColor(int _col) { Set(_col); }

// *** Set
void RGBAColor::Set(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
{
  r = _r;
  g = _g;
  b = _b;
  a = _a;
}

void RGBAColor::Set(int _col)
{
  r = (_col & 0xff000000) >> 24;
  g = (_col & 0x00ff0000) >> 16;
  b = (_col & 0x0000ff00) >> 8;
  a = (_col & 0x000000ff) >> 0;
}

// *** Operator +
RGBAColor RGBAColor::operator +(const RGBAColor& _b) const { return RGBAColor((r + _b.r), (g + _b.g), (b + _b.b), (a + _b.a)); }

// *** Operator -
RGBAColor RGBAColor::operator -(const RGBAColor& _b) const { return RGBAColor((r - _b.r), (g - _b.g), (b - _b.b), (a - _b.a)); }

// *** Operator *
RGBAColor RGBAColor::operator *(const float _b) const
{
  return RGBAColor(static_cast<unsigned char>(static_cast<float>(r) * _b), static_cast<unsigned char>(static_cast<float>(g) * _b),
                    static_cast<unsigned char>(static_cast<float>(b) * _b), static_cast<unsigned char>(static_cast<float>(a) * _b));
}

// *** Operator /
RGBAColor RGBAColor::operator /(const float _b) const
{
  float multiplier = 1.0f / _b;
  return RGBAColor(static_cast<unsigned char>(static_cast<float>(r) * multiplier),
                    static_cast<unsigned char>(static_cast<float>(g) * multiplier),
                    static_cast<unsigned char>(static_cast<float>(b) * multiplier),
                    static_cast<unsigned char>(static_cast<float>(a) * multiplier));
}

// *** Operator *=
const RGBAColor& RGBAColor::operator *=(const float _b)
{
  r = static_cast<unsigned char>(static_cast<float>(r) * _b);
  g = static_cast<unsigned char>(static_cast<float>(g) * _b);
  b = static_cast<unsigned char>(static_cast<float>(b) * _b);
  a = static_cast<unsigned char>(static_cast<float>(a) * _b);
  return *this;
}

// *** Operator /=
const RGBAColor& RGBAColor::operator /=(const float _b)
{
  float multiplier = 1.0f / _b;
  r = static_cast<unsigned char>(static_cast<float>(r) * multiplier);
  g = static_cast<unsigned char>(static_cast<float>(g) * multiplier);
  b = static_cast<unsigned char>(static_cast<float>(b) * multiplier);
  a = static_cast<unsigned char>(static_cast<float>(a) * multiplier);
  return *this;
}

// *** Operator +=
const RGBAColor& RGBAColor::operator +=(const RGBAColor& _b)
{
  r += _b.r;
  g += _b.g;
  b += _b.b;
  a += _b.a;
  return *this;
}

// *** Operator -=
const RGBAColor& RGBAColor::operator -=(const RGBAColor& _b)
{
  r -= _b.r;
  g -= _b.g;
  b -= _b.b;
  a -= _b.a;
  return *this;
}

// *** Operator ==
bool RGBAColor::operator ==(const RGBAColor& _b) const { return (r == _b.r && g == _b.g && b == _b.b && a == _b.a); }

// *** Operator !=
bool RGBAColor::operator !=(const RGBAColor& _b) const { return (r != _b.r || g != _b.g || b != _b.b || a != _b.a); }

// *** GetData
const unsigned char* RGBAColor::GetData() const { return &r; }

// *** AddWithClamp
void RGBAColor::AddWithClamp(const RGBAColor& _b)
{
  float alpha = static_cast<float>(_b.a) / 255.0f;

  int newR = static_cast<int>(r) + static_cast<int>(_b.r * alpha);
  int newG = static_cast<int>(g) + static_cast<int>(_b.g * alpha);
  int newB = static_cast<int>(b) + static_cast<int>(_b.b * alpha);

  newR = std::max(newR, 0);
  newG = std::max(newG, 0);
  newB = std::max(newB, 0);

  newR = std::min(newR, 255);
  newG = std::min(newG, 255);
  newB = std::min(newB, 255);

  r = newR;
  g = newG;
  b = newB;
}

// *** MultiplyWithClamp
void RGBAColor::MultiplyWithClamp(float _scale)
{
  if (static_cast<float>(r) * _scale < 255.0f)
    r = static_cast<unsigned char>(static_cast<float>(r) * _scale);
  else
    r = 255;

  if (static_cast<float>(g) * _scale < 255.0f)
    g = static_cast<unsigned char>(static_cast<float>(g) * _scale);
  else
    g = 255;

  if (static_cast<float>(b) * _scale < 255.0f)
    b = static_cast<unsigned char>(static_cast<float>(b) * _scale);
  else
    b = 255;

  if (static_cast<float>(a) * _scale < 255.0f)
    a = static_cast<unsigned char>(static_cast<float>(a) * _scale);
  else
    a = 255;
}
