#ifndef INCLUDED_RGB_COLOUR_H
#define INCLUDED_RGB_COLOUR_H

class RGBAColor
{
  public:
    unsigned char r, g, b, a;

    RGBAColor();
    RGBAColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
    RGBAColor(int _col);

    void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
    void Set(int _col);

    RGBAColor operator +(const RGBAColor& b) const;
    RGBAColor operator -(const RGBAColor& b) const;
    RGBAColor operator *(float b) const;
    RGBAColor operator /(float b) const;

    RGBAColor& operator =(const RGBAColor& _b) = default;
    const RGBAColor& operator *=(float b);
    const RGBAColor& operator /=(float b);
    const RGBAColor& operator +=(const RGBAColor& b);
    const RGBAColor& operator -=(const RGBAColor& b);

    bool operator ==(const RGBAColor& b) const;
    bool operator !=(const RGBAColor& b) const;

    const unsigned char* GetData() const;

    void AddWithClamp(const RGBAColor& b);
    void MultiplyWithClamp(float _scale);
};

// Operator * between float and RGBAColor
inline RGBAColor operator *(float _scale, const RGBAColor& _v) { return _v * _scale; }

extern RGBAColor g_colourBlack;
extern RGBAColor g_colourWhite;

#endif
