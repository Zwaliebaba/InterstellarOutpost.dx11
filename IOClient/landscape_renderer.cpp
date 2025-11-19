#include "pch.h"
#include "landscape_renderer.h"
#include "2d_surface_map.h"
#include "LegacyVector3.h"
#include "GameApp.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "level_file.h"
#include "location.h"
#include "math_utils.h"
#include "ogl_extensions.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "rgb_colour.h"
#include "texture_uv.h"
#include "opengl_directx_internals.h"

void LandscapeRenderer::BuildVertArrayAndTriStrip(const SurfaceMap2D<float>* _heightMap)
{
  m_highest = _heightMap->GetHighestValue();

  const int rows = _heightMap->GetNumColumns();
  const int cols = _heightMap->GetNumRows();

  m_verts.SetStepDouble();
  auto strip = NEW LandTriangleStrip();
  strip->m_firstVertIndex = 0;
  int degen = 0;
  LandVertex vertex1, vertex2;

  for (int z = 0; z < cols; ++z)
  {
    float fz = static_cast<float>(z) * _heightMap->m_cellSizeY;

    for (int x = 0; x < rows; ++x)
    {
      float heighta = _heightMap->GetData(x - 1, z);
      float heightb = _heightMap->GetData(x - 1, z + 1);
      float height1 = _heightMap->GetData(x, z);
      float height2 = _heightMap->GetData(x, z + 1);
      float height3 = _heightMap->GetData(x + 1, z);
      float height4 = _heightMap->GetData(x + 1, z + 1);
      // Is this quad at least partly above water?
      if (heighta > 0.0f || heightb > 0.0f || height1 > 0.0f || height2 > 0.0f || height3 > 0.0f || height4 > 0.0f)
      {
        // Yes it is, add it to the strip
        float fx = static_cast<float>(x) * _heightMap->m_cellSizeX;
        vertex1.m_pos.Set(fx, height1, fz);
        vertex2.m_pos.Set(fx, height2, fz + _heightMap->m_cellSizeY);
        if (vertex1.m_pos.y < 0.3f)
          vertex1.m_pos.y = -10.0f;
        if (vertex2.m_pos.y < 0.3f)
          vertex2.m_pos.y = -10.0f;
        if (degen == 1)
        {
          m_verts.PutData(vertex1);
          m_verts.PutData(vertex1);
        }
        degen = 2;
        m_verts.PutData(vertex1);
        m_verts.PutData(vertex2);
      }
      else
      {
        // No, quad is entirely below water, add degenerated joint.
        if (degen == 2)
        {
          m_verts.PutData(vertex2);
          m_verts.PutData(vertex2);
          degen = 1;
        }
      }
    }
  }

  // end strip
  strip->m_numVerts = m_verts.NumUsed();
  m_strips.PutData(strip);
  m_numTriangles = strip->m_numVerts - 2;
}

void LandscapeRenderer::BuildNormArray()
{
  if (m_verts.Size() <= 0)
    return;

  int nextNormId = 0;

  // Go through all the strips...
  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    m_verts[nextNormId++].m_norm = g_upVector;
    m_verts[nextNormId++].m_norm = g_upVector;
    const int maxJ = strip->m_numVerts - 2;

    // For each vertex in strip
    for (int j = 0; j < maxJ; j += 2)
    {
      XMVECTOR v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
      XMVECTOR v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
      XMVECTOR v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
      XMVECTOR v4 = m_verts[strip->m_firstVertIndex + j + 3].m_pos;

      LegacyVector3 north(v1 - v2);
      LegacyVector3 northEast(v3 - v2);
      LegacyVector3 east(v4 - v2);

      LegacyVector3 norm1(northEast ^ north);
      norm1.Normalize();
      LegacyVector3 norm2(east ^ northEast);
      norm2.Normalize();

      m_verts[nextNormId++].m_norm = norm1;
      m_verts[nextNormId++].m_norm = norm2;

      int vertIndex = strip->m_firstVertIndex + j + 2;
      DEBUG_ASSERT(nextNormId - 2 == vertIndex);
    }
  }

  int vertIndex = m_verts.NumUsed();
  DEBUG_ASSERT(nextNormId == vertIndex);
}

void LandscapeRenderer::BuildUVArray(SurfaceMap2D<float>* _heightMap)
{
  if (m_verts.Size() <= 0)
    return;

  int nextUVId = 0;

  float factorX = 1.0f / _heightMap->m_cellSizeX;
  float factorZ = 1.0f / _heightMap->m_cellSizeY;

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    const int numVerts = strip->m_numVerts;
    for (int j = 0; j < numVerts; ++j)
    {
      const LegacyVector3& vert = m_verts[strip->m_firstVertIndex + j].m_pos;
      float u = vert.x * factorX;
      float v = vert.z * factorZ;
      m_verts[nextUVId++].m_uv = TextureUV(u, v);
    }
  }

  DEBUG_ASSERT(nextUVId == m_verts.NumUsed());
}

// _gradient=1 means flat, _gradient=0 means vertical
// x and z are only passed in as a means to get predicatable noise
void LandscapeRenderer::GetLandscapeColor(float _height, float _gradient, unsigned int _x, unsigned int _y, RGBAColor* _colour)
{
  float heightAboveSea = _height;
  float u = powf(1.0f - _gradient, 0.4f);
  float v = 1.0f - heightAboveSea / m_highest;
  darwiniaSeedRandom(_x | _y + darwiniaRandom());
  if (heightAboveSea < 0.0f)
    heightAboveSea = 0.0f;
  v += sfrand(0.45f / (heightAboveSea + 2.0f));

  int x = u * m_landscapeColor->m_width;
  int y = v * m_landscapeColor->m_height;

  if (x < 0)
    x = 0;
  if (x > m_landscapeColor->m_width - 1)
    x = m_landscapeColor->m_width - 1;
  if (y < 0)
    y = 0;
  if (y > m_landscapeColor->m_height - 1)
    y = m_landscapeColor->m_height - 1;

  *_colour = m_landscapeColor->GetPixel(x, y);
}

void LandscapeRenderer::BuildColorArray()
{
  if (m_verts.Size() <= 0)
    return;

  int nextColId = 0;

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    m_verts[nextColId++].m_col.Set(255, 0, 255);
    m_verts[nextColId++].m_col.Set(255, 0, 255);

    const int numVerts = strip->m_numVerts - 2;
    for (int j = 0; j < numVerts; ++j)
    {
      const LegacyVector3& v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
      const LegacyVector3& v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
      const LegacyVector3& v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
      LegacyVector3 center = (v1 + v2 + v3) * 0.33333f;
      const LegacyVector3& norm = m_verts[strip->m_firstVertIndex + j + 2].m_norm;
      RGBAColor col;
      GetLandscapeColor(center.y, norm.y, static_cast<unsigned int>(center.x), static_cast<unsigned int>(center.z), &col);

      D3DCOLOR d3dCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);
      memcpy(&m_verts[nextColId++].m_col, &d3dCol, sizeof(d3dCol));

      if (j == 0)
      {
        // We need to set the colours of the first two verticies to something
        // other than bright pink. Why this isn't a problem in OpenGL I
        // don't know.
        memcpy(&m_verts[nextColId - 3].m_col, &d3dCol, sizeof(d3dCol));
        memcpy(&m_verts[nextColId - 2].m_col, &d3dCol, sizeof(d3dCol));
      }
    }
  }

  DEBUG_ASSERT(nextColId == m_verts.NumUsed());
}

const unsigned LandscapeRenderer::m_posOffset(0);
const unsigned LandscapeRenderer::m_normOffset(sizeof(LegacyVector3));
const unsigned LandscapeRenderer::m_colOffset(sizeof(LegacyVector3) * 2);
const unsigned LandscapeRenderer::m_uvOffset(sizeof(LegacyVector3) * 2 + sizeof(RGBAColor));

LandscapeRenderer::LandscapeRenderer(SurfaceMap2D<float>* _heightMap)
  : m_vertexBuffer(0)
{
  char fullFilname[256];
  sprintf(fullFilname, "terrain\\%s", g_app->m_location->m_levelFile->m_landscapeColorFilename.c_str());

  if (Location::ChristmasModEnabled() == 1)
    strcpy(fullFilname, "terrain\\landscape_icecaps.bmp");

  BinaryReader* reader = Resource::GetBinaryReader(fullFilname);
  ASSERT_TEXT(reader != NULL, "Failed to get resource {}", fullFilname);
  m_landscapeColor = NEW BitmapRGBA(reader, "bmp");
  delete reader;

  BuildOpenGlState(_heightMap);
}

LandscapeRenderer::~LandscapeRenderer()
{
  m_verts.Empty();

#ifdef USE_DIRECT3D
  ReleaseD3DResources();
#endif
}

#ifdef USE_DIRECT3D

static LPDIRECT3DVERTEXDECLARATION9 s_vertexDecl = nullptr;

void LandscapeRenderer::ReleaseD3DPoolDefaultResources() { SAFE_RELEASE(s_vertexDecl); }

void LandscapeRenderer::ReleaseD3DResources()
{
  ReleaseD3DPoolDefaultResources();
  if (m_vertexBuffer)
  {
    gglDeleteBuffersARB(1, &m_vertexBuffer);
    m_vertexBuffer = 0;
  }
}

static LPDIRECT3DVERTEXDECLARATION9 GetVertexDecl()
{
  static D3DVERTEXELEMENT9 s_vertexDesc[] = {
    {0, LandscapeRenderer::m_posOffset, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, LandscapeRenderer::m_normOffset, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
    {0, LandscapeRenderer::m_colOffset, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    {0, LandscapeRenderer::m_uvOffset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0} // D3DDECL_END
  };

  if (!s_vertexDecl)
    OpenGLD3D::g_pd3dDevice->CreateVertexDeclaration(s_vertexDesc, &s_vertexDecl);

  return s_vertexDecl;
}

#endif

void LandscapeRenderer::BuildOpenGlState(SurfaceMap2D<float>* _heightMap)
{
  BuildVertArrayAndTriStrip(_heightMap);
  BuildNormArray();
  BuildColorArray();
  BuildUVArray(_heightMap);

  // Flip the normals for Direct3D
  const int numUsed = m_verts.NumUsed();
  for (int i = 0; i < numUsed; ++i)
    m_verts[i].m_norm *= -1;

  if (m_verts.NumUsed() <= 0)
    return;

  DEBUG_ASSERT(!m_vertexBuffer);
  gglGenBuffersARB(1, &m_vertexBuffer);
  gglBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBuffer);
  gglBufferDataARB(GL_ARRAY_BUFFER_ARB, m_verts.Size() * sizeof(LandVertex), m_verts.GetPointer(0), GL_STATIC_DRAW_ARB);
  gglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void LandscapeRenderer::RenderMain()
{
  GLfloat materialSpecular[] = {0.0f, 0.0f, 0.0f, 0.0f};
  GLfloat materialDiffuse[] = {1.0f, 1.0f, 1.0f, 0.0f};
  GLfloat materialShininess[] = {100.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_LIGHTING);

  DEBUG_ASSERT(m_vertexBuffer);
  gglBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBuffer);
  OpenGLD3D::g_pd3dDevice->SetVertexDeclaration(GetVertexDecl());
  OpenGLD3D::g_pd3dDevice->SetStreamSource(0, *OpenGLD3D::g_currentVertexBuffer, 0, sizeof(LandVertex));

  for (int z = 0; z < m_strips.Size(); ++z)
  {
    LandTriangleStrip* strip = m_strips[z];
    if (strip->m_numVerts > 2)
      OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, strip->m_firstVertIndex, strip->m_numVerts - 2);
  }

  gglBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
}

void LandscapeRenderer::RenderOverlay()
{
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glDepthMask(false);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

  int outlineTextureId = Resource::GetTexture("textures\\triangleOutline.bmp", true, false);
  glBindTexture(GL_TEXTURE_2D, outlineTextureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  GLfloat materialSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
  GLfloat materialDiffuse[] = {1.2f, 1.2f, 1.2f, 0.0f};
  GLfloat materialShininess[] = {40.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);

  DEBUG_ASSERT(m_vertexBuffer);
  gglBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBuffer);
  OpenGLD3D::g_pd3dDevice->SetVertexDeclaration(GetVertexDecl());
  OpenGLD3D::g_pd3dDevice->SetStreamSource(0, *OpenGLD3D::g_currentVertexBuffer, 0, sizeof(LandVertex));

  for (int z = 0; z < m_strips.Size(); ++z)
  {
    LandTriangleStrip* strip = m_strips[z];

    OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, strip->m_firstVertIndex, strip->m_numVerts - 2);
  }

  gglBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDepthMask(true);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void LandscapeRenderer::Render()
{
  if (m_verts.Size() <= 0)
    return;

  g_app->m_location->SetupFog();
  glEnable(GL_FOG);

  START_PROFILE(g_app->m_profiler, "Render Landscape Main");
  RenderMain();
  END_PROFILE(g_app->m_profiler, "Render Landscape Main");

    START_PROFILE(g_app->m_profiler, "Render Landscape Overlay");
    RenderOverlay();
    END_PROFILE(g_app->m_profiler, "Render Landscape Overlay");

  glDisable(GL_FOG);
}
