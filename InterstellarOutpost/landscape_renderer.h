#pragma once

#include "2d_surface_map.h"
#include "LegacyVector3.h"
#include "fast_darray.h"
#include "rgb_colour.h"
#include "texture_uv.h"

class BitmapRGBA;

class LandVertex
{
  public:
    LegacyVector3 m_pos;
    LegacyVector3 m_norm;
    RGBAColor m_col;
    TextureUV m_uv;
};

class LandTriangleStrip
{
  public:
    int m_firstVertIndex;
    int m_numVerts;

    LandTriangleStrip()
      : m_firstVertIndex(-1),
        m_numVerts(-2) {}
};

//*****************************************************************************
// Class LandscapeRenderer
//*****************************************************************************

struct IDirect3DVertexBuffer9;

class LandscapeRenderer
{
  protected:
    enum
    {
      RenderModeVertexArray,
      RenderModeDisplayList,
      RenderModeVertexBufferObject,
      RenderModeVertexBufferDirect3D
    };

    BitmapRGBA* m_landscapeColor;
    float m_highest;

    FastDArray<LandVertex> m_verts;

    unsigned int m_vertexBuffer;

    FastDArray<LandTriangleStrip*> m_strips;

    void BuildVertArrayAndTriStrip(const SurfaceMap2D<float>* _heightMap);
    void BuildNormArray();
    void BuildUVArray(SurfaceMap2D<float>* _heightMap);
    void GetLandscapeColor(float _height, float _gradient, unsigned int _x, unsigned int _y, RGBAColor* _colour);
    void BuildColorArray();

  public:
    static const unsigned int m_posOffset;
    static const unsigned int m_normOffset;
    static const unsigned int m_colOffset;
    static const unsigned int m_uvOffset;

    int m_numTriangles;

    LandscapeRenderer(SurfaceMap2D<float>* _heightMap);
    ~LandscapeRenderer();

    void ReleaseD3DPoolDefaultResources();
    void ReleaseD3DResources();

    void BuildOpenGlState(SurfaceMap2D<float>* _heightMap);

    void RenderMain();
    void RenderOverlay();
    void Render();
};
