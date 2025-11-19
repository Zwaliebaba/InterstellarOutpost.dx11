#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

class RGBAColor;
class jpeg_decoder;
class BitmapFileHeader;
class BitmapInfoHeader;
class BinaryReader;

class BitmapRGBA
{
  void ReadBMPFileHeader(BinaryReader* f, BitmapFileHeader* fileheader);
  void ReadWinBMPInfoHeader(BinaryReader* f, BitmapInfoHeader* infoheader);
  void ReadOS2BMPInfoHeader(BinaryReader* f, BitmapInfoHeader* infoheader);

  void ReadBMPPalette(int ncols, RGBAColor pal[256], BinaryReader* f, int win_flag);
  void Read4BitLine(int length, BinaryReader* f, RGBAColor* pal, int line);
  void Read8BitLine(int length, BinaryReader* f, RGBAColor* pal, int line);
  void Read24BitLine(int length, BinaryReader* f, int line);
  void LoadBmp(BinaryReader* _in);

  public:
    int m_width;
    int m_height;
    RGBAColor* m_pixels;
    RGBAColor** m_lines;

    BitmapRGBA();
    BitmapRGBA(const BitmapRGBA& _other);
    BitmapRGBA(int _width, int _height);
    BitmapRGBA(const char* _filename);
    BitmapRGBA(BinaryReader* _reader, const char* _type);
    ~BitmapRGBA();

    void Initialize(int _width, int _height);
    void Initialize(const char* _filename);
    void Initialize(BinaryReader* _reader, const char* _type);

    void Clear(const RGBAColor& colour);

    void PutPixel(int x, int y, const RGBAColor& colour);
    const RGBAColor& GetPixel(int x, int y) const;

    const RGBAColor& GetPixelClipped(int x, int y) const;

    RGBAColor GetInterpolatedPixel(float x, float y) const;

    void Blit(int srcX, int srcY, int srcW, int srcH, const BitmapRGBA* _srcBmp, int destX, int destY, int destW, int destH,
              bool _bilinear);

    void ApplyBlurFilter(float _scale);

    void ConvertPinkToTransparent();
    int ConvertToTexture(bool _mipmapping = true) const;
};

#endif
