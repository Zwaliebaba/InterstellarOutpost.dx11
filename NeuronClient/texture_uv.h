#ifndef INCLUDED_TEXTURE_UV_H
#define INCLUDED_TEXTURE_UV_H

class TextureUV
{
  public:
    float u, v;

    TextureUV();
    TextureUV(float u, float v);

    void Set(float u, float v);

    TextureUV operator +(const TextureUV& b) const;
    TextureUV operator -(const TextureUV& b) const;
    TextureUV operator *(float b) const;
    TextureUV operator /(float b) const;

    const TextureUV& operator =(const TextureUV& b);
    const TextureUV& operator *=(float b);
    const TextureUV& operator /=(float b);
    const TextureUV& operator +=(const TextureUV& b);
    const TextureUV& operator -=(const TextureUV& b);

    bool operator ==(const TextureUV& b) const;
    bool operator !=(const TextureUV& b) const;

    float* GetData();
};

// Operator * between float and TextureUV
inline TextureUV operator *(float _scale, const TextureUV& _v) { return _v * _scale; }

#endif
