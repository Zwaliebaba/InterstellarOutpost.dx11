#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

class RGBAColor;
class LegacyVector3;

#define DEF_FONT_SIZE 12.0f

class DX9TextRenderer : protected ASyncLoader
{
public:
  void Startup(const std::wstring& _filename);
  void Shutdown();

  fire_and_forget LoadAssetsAsync();

  void BeginText2D();
  void EndText2D();

  void SetRenderShadow(bool _renderShadow);

  void DrawText2DSimple(float _x, float _y, float _size, std::string_view _text);
  void DrawText2D(float _x, float _y, float _size, std::string_view _text, ...);
  void DrawText2DRight(float _x, float _y, float _size, std::string_view _text, ...);
  void DrawText2DCenter(float _x, float _y, float _size, std::string_view _text, ...);

  void DrawText3DSimple(const LegacyVector3& _pos, float _size, std::string_view _text);
  void DrawText3D(const LegacyVector3& _pos, float _size, std::string_view _text, ...);
  void DrawText3DCenter(const LegacyVector3& _pos, float _size, std::string_view _text, ...);

  void DrawText3D(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up, float _size,
    std::string_view _text, ...);

  static float GetTextWidth(size_t _numChars, float _size = 13.0f);

protected:
  float m_projectionMatrix[16] = {};
  float m_modelviewMatrix[16] = {};
  std::wstring m_filename;
  std::weak_ptr<Texture> m_textureID;
  bool m_renderShadow = {};

  static float GetTexCoordX(unsigned char theChar);
  static float GetTexCoordY(unsigned char theChar);
};

inline DX9TextRenderer g_gameFont;
inline DX9TextRenderer g_editorFont;

#endif
