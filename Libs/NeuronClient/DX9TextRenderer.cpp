#include "pch.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "Camera.h"

// Horizontal size as a proportion of vertical size
constexpr float HORIZONTAL_SIZE = 0.6f;

constexpr float TEX_MARGIN = 0.003f;
constexpr float TEX_STRETCH = 1.0f - 26.0f * TEX_MARGIN;

constexpr float TEX_WIDTH = 1.0f / 16.0f * TEX_STRETCH * 0.9f;
constexpr float TEX_HEIGHT = 1.0f / 14.0f * TEX_STRETCH;

void DX9TextRenderer::Startup(const std::wstring& _filename)
{
  m_filename = _filename;
  LoadAssetsAsync();
}

void DX9TextRenderer::Shutdown()
{
}

fire_and_forget DX9TextRenderer::LoadAssetsAsync()
{
  StartLoading();
  co_await resume_background();
  m_textureID = TextureManager::LoadTexture(m_filename);
  FinishLoading();
}

void DX9TextRenderer::BeginText2D()
{
  WaitForLoad();

  GLint matrixMode;
  GLint v[4];

  /* Setup OpenGL matrices */
  glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
  glGetIntegerv(GL_VIEWPORT, &v[0]);

  glMatrixMode(GL_MODELVIEW);
  glGetDoublev(GL_MODELVIEW_MATRIX, m_modelviewMatrix);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glGetDoublev(GL_PROJECTION_MATRIX, m_projectionMatrix);
  glLoadIdentity();
  gluOrtho2D(v[0] - 0.325, v[0] + v[2] - 0.325, v[1] + v[3] - 0.325, v[1] - 0.325);

  glDisable(GL_LIGHTING);
  glColor(Color::WHITE);

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(false);

  glMatrixMode(matrixMode);
}

void DX9TextRenderer::EndText2D()
{
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixd(m_projectionMatrix);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixd(m_modelviewMatrix);

  glDepthMask(true);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
}

float DX9TextRenderer::GetTexCoordX(unsigned char theChar)
{
  constexpr float CHAR_WIDTH = 1.0f / 16.0f;
  const float xPos = theChar % 16;
  const float texX = xPos * CHAR_WIDTH + TEX_MARGIN + 0.002f;
  return texX;
}

float DX9TextRenderer::GetTexCoordY(unsigned char theChar)
{
  constexpr float CHAR_HEIGHT = 1.0f / 14.0f;
  const float yPos = (theChar >> 4) - 2;
  const float texY = yPos * CHAR_HEIGHT + TEX_MARGIN + 0.001f;
  return texY;
}

void DX9TextRenderer::SetRenderShadow(bool _renderShadow) { m_renderShadow = _renderShadow; }

void DX9TextRenderer::DrawText2DSimple(float _x, float _y, float _size, std::string_view _text)
{
  // Compatibility wank - needed to achieve the same behaviour the old code had
  _y -= 7.0f;
  _x -= 3.0f;

  float horiSize = _size * HORIZONTAL_SIZE;

  if (m_renderShadow)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  const auto text = m_textureID.lock();
  DEBUG_ASSERT(text != nullptr);
  glBindTexture(GL_TEXTURE_2D, text->m_ref);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  size_t numChars = _text.size();
  for (unsigned int i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = _text[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      glBegin(GL_QUADS);

      glTexCoord2f(texX, texY + TEX_HEIGHT);
      glVertex2f(_x, _y + _size);

      glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);
      glVertex2f(_x + horiSize, _y + _size);

      glTexCoord2f(texX + TEX_WIDTH, texY);
      glVertex2f(_x + horiSize, _y);

      glTexCoord2f(texX, texY);
      glVertex2f(_x, _y);

      glEnd();
    }

    _x += horiSize;
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glDisable       ( GL_BLEND );                             // Not here, Blending is enabled duringCanvas::Eclipse render
  glDisable(GL_TEXTURE_2D);
}

void DX9TextRenderer::DrawText2D(float _x, float _y, float _size, std::string_view _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text.data(), ap);
  DrawText2DSimple(_x, _y, _size, buf);
}

void DX9TextRenderer::DrawText2DRight(float _x, float _y, float _size, std::string_view _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text.data(), ap);

  float width = GetTextWidth(strlen(buf), _size);
  DrawText2DSimple(_x - width, _y, _size, buf);
}

void DX9TextRenderer::DrawText2DCenter(float _x, float _y, float _size, std::string_view _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text.data(), ap);

  float width = GetTextWidth(strlen(buf), _size);
  DrawText2DSimple(_x - width / 2, _y, _size, buf);
}

namespace
{
  class SaveGLEnabled
  {
  public:
    SaveGLEnabled(GLenum _what)
      : m_what(_what),
      m_value(glIsEnabled(_what)) {
    }

    ~SaveGLEnabled()
    {
      if (m_value)
        glEnable(m_what);
      else
        glDisable(m_what);
    }

  private:
    GLenum m_what;
    bool m_value;
  };

  class SaveGLTex2DParamI
  {
  public:
    SaveGLTex2DParamI(GLenum _what)
      : m_what(_what) {
      glGetTexParameteriv(GL_TEXTURE_2D, _what, &m_value);
    }

    ~SaveGLTex2DParamI() { glTexParameteri(GL_TEXTURE_2D, m_what, m_value); }

  private:
    GLenum m_what;
    GLint m_value;
  };

  class SaveGLBlendFunc
  {
  public:
    SaveGLBlendFunc()
    {
      glGetIntegerv(GL_BLEND_SRC, &m_srcFactor);
      glGetIntegerv(GL_BLEND_DST, &m_dstFactor);
    }

    ~SaveGLBlendFunc() { glBlendFunc(m_srcFactor, m_dstFactor); }

  private:
    GLint m_srcFactor, m_dstFactor;
  };

  class SaveGLColor
  {
  public:
    SaveGLColor() { glGetFloatv(GL_CURRENT_COLOR, m_color); }

    ~SaveGLColor() { glColor4fv(m_color); }

  private:
    float m_color[4];
  };

  class SaveGLFontDrawAttributes
  {
  public:
    SaveGLFontDrawAttributes()
      : m_textureMagFilter(GL_TEXTURE_MAG_FILTER),
      m_textureMinFilter(GL_TEXTURE_MIN_FILTER),
      m_textureEnabled(GL_TEXTURE_2D),
      m_depthTestEnabled(GL_DEPTH_TEST),
      m_cullFaceEnabled(GL_CULL_FACE),
      m_lightingEnabled(GL_LIGHTING),
      m_blendEnabled(GL_BLEND) {
    }

  private:
    SaveGLColor m_color;
    SaveGLTex2DParamI m_textureMagFilter, m_textureMinFilter;
    SaveGLBlendFunc m_blendFunc;
    SaveGLEnabled m_textureEnabled, m_depthTestEnabled, m_cullFaceEnabled, m_lightingEnabled, m_blendEnabled;
  };
}

void DX9TextRenderer::DrawText3DSimple(const LegacyVector3& _pos, float _size, std::string_view _text)
{
  // Store the GL state
  SaveGLFontDrawAttributes saveAttribs;

  Camera* cam = g_app->m_camera;
  LegacyVector3 pos(_pos);
  LegacyVector3 vertSize = cam->GetUp() * _size;
  LegacyVector3 horiSize = -cam->GetRight() * _size * HORIZONTAL_SIZE;
  size_t numChars = _text.size();
  pos += vertSize * 0.5f;

  //
  // Render the text

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);

  const auto text = m_textureID.lock();
  DEBUG_ASSERT(text != nullptr);
  glBindTexture(GL_TEXTURE_2D, text->m_ref);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  if (m_renderShadow)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  for (size_t i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = _text[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      glBegin(GL_QUADS);
      glTexCoord2f(texX, texY + TEX_HEIGHT);
      glVertex3fv((pos - vertSize).GetData());
      glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);
      glVertex3fv((pos + horiSize - vertSize).GetData());
      glTexCoord2f(texX + TEX_WIDTH, texY);
      glVertex3fv((pos + horiSize).GetData());
      glTexCoord2f(texX, texY);
      glVertex3fv((pos).GetData());
      glEnd();
    }

    pos += horiSize;
  }
}

void DX9TextRenderer::DrawText3D(const LegacyVector3& _pos, float _size, std::string_view _text, ...)
{
  // Convert the variable argument list into a single string
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text.data(), ap);

  DrawText3DSimple(_pos, _size, buf);
}

void DX9TextRenderer::DrawText3DCenter(const LegacyVector3& _pos, float _size, std::string_view _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text.data(), ap);

  LegacyVector3 pos = _pos;
  float amount = HORIZONTAL_SIZE * static_cast<float>(strlen(buf)) * 0.5f * _size;
  pos += g_app->m_camera->GetRight() * amount;

  DrawText3DSimple(pos, _size, buf);
}

void DX9TextRenderer::DrawText3D(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up, float _size,
  std::string_view _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text.data(), ap);

  SaveGLFontDrawAttributes saveAttribs;

  Camera* cam = g_app->m_camera;

  LegacyVector3 pos = _pos;
  LegacyVector3 vertSize = _up * _size;
  LegacyVector3 horiSize = (_up ^ _front) * _size * HORIZONTAL_SIZE;
  size_t numChars = strlen(buf);
  pos -= horiSize * numChars * 0.5f;
  pos += vertSize * 0.5f;

  //
  // Render the text

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);

  glEnable(GL_TEXTURE_2D);

  const auto text = m_textureID.lock();
  DEBUG_ASSERT(text != nullptr);
  glBindTexture(GL_TEXTURE_2D, text->m_ref);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  if (m_renderShadow)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  for (size_t i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = buf[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      glBegin(GL_QUADS);
      glTexCoord2f(texX, texY + TEX_HEIGHT);
      glVertex3fv((pos - vertSize).GetData());
      glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);
      glVertex3fv((pos + horiSize - vertSize).GetData());
      glTexCoord2f(texX + TEX_WIDTH, texY);
      glVertex3fv((pos + horiSize).GetData());
      glTexCoord2f(texX, texY);
      glVertex3fv(pos.GetData());
      glEnd();
    }

    pos += horiSize;
  }
}

float DX9TextRenderer::GetTextWidth(size_t _numChars, float _size) { return _numChars * _size * HORIZONTAL_SIZE; }
