#include "pch.h"
#include "GameApp.h"
#include "landscape.h"
#include "landscape_renderer.h"
#include "location.h"
#include "ogl_extensions.h"
#include "opengl_directx_dlist.h"
#include "opengl_directx_dlist_dev.h"
#include "opengl_directx_internals.h"
#include "opengl_directx_matrix_stack.h"
#include "resource.h"
#include "input.h"

using namespace OpenGLD3D;

#define FRAMES_PER_SECOND_COUNTER
//#define D3DTS_WORLD D3DTS_VIEW

// Notes:
// OpenGL uses a RH coordinate system (+ve Z goes into the monitor),
// Direct3D uses LH coordinate system (-ve Z goes into the monitor)
// Matrix format is different in the two systems (One is the transpose of the other)
// gluLookAt -> D3DXMatrixLookAtRH (LH) ?
// gluOrtho, gluOrtho2D -> D3DXMatrixOrthoRH (LH) ?

/* TODO:

	- Implement GL_FOG_HINT and GL_POLYGON_HINT properly.
	- Proper error reporting in glGetError, gluErrorString
	- Line width emulation


	. Create matrix stack class, with modified flag, transform type (View / Projection)
		- constructor to get the current device transform
			- initialise with D3DTRANSFORMSTATETYPE
		- destructor so that it releases the internal matrix stack
		- Load
		- Multiply
		- Modified			(need to set the transform to sync up)
		- SetTransform		(set the device)
		- GetTransform

		- Look in the code for all ->SetTransform and ->GetTransform and recode as appropriate
*/

namespace OpenGLD3D
{
  LPDIRECT3D9 g_pD3D = nullptr; // Used to create the D3DDevice
  D3DPRESENT_PARAMETERS g_d3dpp;			 // The device parameters

  LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
  LPDIRECT3DDEVICE9 g_pd3dDeviceActual = nullptr; // The device corresponding to the
  bool g_supportsHwVertexProcessing = true;
  bool g_supportsAutoMipmapping = true;

  struct TextureState
  {
    GLuint target;
    GLuint lastTarget;
    D3DCOLORVALUE envColor;
    GLenum envMode;
    GLenum lastEnvMode;
    GLenum rgbCombineMode;

    TextureState()
      : target(0),
        lastTarget(-1),
        envMode(GL_MODULATE),
        lastEnvMode(0),
        rgbCombineMode(0)
    {
      ZeroMemory(&envColor, sizeof(envColor));
    }
  };
}

// Misc
static D3DCULL s_cullMode = D3DCULL_CW;
static bool s_ccwFrontFace = true;
static bool s_cullBackFace = true;

// Matrix and Projection
static GLenum s_matrixMode = GL_MODELVIEW;
static auto s_targetMatrixTransformType = D3DTS_WORLD;
static MatrixStack* s_pTargetMatrixStack = nullptr;

static MatrixStack* s_pModelViewMatrixStack = nullptr;
static MatrixStack* s_pProjectionMatrixStack = nullptr;

static MatrixStack* s_pModelViewMatrixStackActual = nullptr;
static MatrixStack* s_pProjectionMatrixStackActual = nullptr;

// Colors
static D3DCOLOR s_clearColor;

// Primitives
static GLenum s_primitiveMode = GL_QUADS;

// Vertices

static D3DVERTEXELEMENT9 s_customVertexDesc[] = {
  {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
  {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
  {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
  {0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
  {0, 36, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
  {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0}// D3DDECL_END
};

LPDIRECT3DVERTEXDECLARATION9 s_pCustomVertexDecl;

static CustomVertex* s_vertices = nullptr;
static CustomVertex* s_currentVertex = nullptr;
static unsigned s_allocatedVertices = 0;
static unsigned s_currentVertexNumber = 0;

// Textures
static std::vector<LPDIRECT3DTEXTURE9> s_textureIds;

// LegacyTexture Samplers
constexpr int MAX_ACTIVE_TEXTURES = 2;
unsigned s_activeTexture = 0;
static TextureState s_textureStates[MAX_ACTIVE_TEXTURES];
static TextureState* s_activeTextureState = &s_textureStates[0];

// Display lists
static std::map<unsigned, DisplayList*> s_displayLists;
static unsigned s_lastDisplayId = 0;
static DisplayListDevice* s_pDisplayListDevice = nullptr;

// Saved Attributes
class CurrentAttributes : public CustomVertex
{
  public:
    CurrentAttributes();

    // Current raster position
    // GL_CURRENT_RASTER_POSITION_VALID flag
    // RGBA color associated with current raster position
    // Color index associated with current raster position
    // LegacyTexture coordinates associated with current raster position
    // GL_EDGE_FLAG flag

    float alphaRef;				// Store precise value of alpha ref

    bool texturingEnabled[MAX_ACTIVE_TEXTURES];		//			(GL_ENABLE_BIT)
    bool backCullingEnabled;		//								(GL_CULL_FACE)

    bool colorMaterialEnabled;	// Material properties			(GL_ENABLE_BIT, GL_LIGHTING_BIT)
    GLenum colorMaterialMode;		//								(GL_LIGHTING_BIT)
};

static CurrentAttributes s_currentAttribs;

// Attribute Stack
class Attributes
{
  public:
    Attributes(const CurrentAttributes& _currentAttribs, IDirect3DStateBlock9* _state);

    CurrentAttributes m_currentAttribs;
    IDirect3DStateBlock9* m_d3dState;
};

static std::stack<Attributes> s_attributeStack;

// --- Attributes -----------------------------------------------------------------

Attributes::Attributes(const CurrentAttributes& _currentAttribs, IDirect3DStateBlock9* _state)
  : m_currentAttribs(_currentAttribs),
    m_d3dState(_state) {}

// --- CurrentAttributes ----------------------------------------------------------

CurrentAttributes::CurrentAttributes()
{
  ZeroMemory(this, sizeof(*this));
  colorMaterialMode = GL_DIFFUSE;
  a8 = 255;
}

// --- The rest -------------------------------------------------------------------

static void InitialiseData()
{
  D3DXMATRIX m;

  // Matrix Data

  if (s_pModelViewMatrixStackActual != nullptr)
  {
    delete s_pModelViewMatrixStackActual;
    delete s_pProjectionMatrixStackActual;
  }

  s_pModelViewMatrixStackActual = NEW MatrixStackActual(D3DTS_WORLD);
  s_pProjectionMatrixStackActual = NEW MatrixStackActual(D3DTS_PROJECTION);

  s_matrixMode = GL_MODELVIEW;
  s_targetMatrixTransformType = D3DTS_WORLD;
  s_pModelViewMatrixStack = s_pModelViewMatrixStackActual;
  s_pProjectionMatrixStack = s_pProjectionMatrixStackActual;
  s_pTargetMatrixStack = s_pModelViewMatrixStack;

  // Colors
  s_clearColor = D3DCOLOR_RGBA(0, 0, 0, 255);

  // Vertex data

  g_pd3dDevice->CreateVertexDeclaration(s_customVertexDesc, &s_pCustomVertexDecl);

  if (s_vertices == nullptr)
  {
    s_allocatedVertices = 1024;
    s_vertices = NEW CustomVertex[s_allocatedVertices];
  }
  s_currentVertexNumber = 0;
  s_currentVertex = s_vertices;

  // Textures
  s_textureIds.clear();
  s_activeTexture = 0;
  s_activeTextureState = &s_textureStates[s_activeTexture];
  for (int i = 0; i < MAX_ACTIVE_TEXTURES; i++)
    s_textureStates[i] = TextureState();

  // Misc
  s_ccwFrontFace = true;
  s_cullBackFace = true;
  s_cullMode = D3DCULL_CW;

  // Current
  s_currentAttribs = CurrentAttributes();
}

// d3d pool default resources are mostly automatically created
//  (on misc places) first time they are needed
void CreateD3dPoolDefaultResources()
{
}

// to be called before d3d reset
void ReleaseD3DPoolDefaultResources()
{
  if (g_app && g_app->m_location && g_app->m_location->m_landscape.m_renderer)
    g_app->m_location->m_landscape.m_renderer->ReleaseD3DPoolDefaultResources();
  for (int i = 0; i < s_textureIds.size(); i++) { SAFE_RELEASE(s_textureIds[i]); }
}

// d3d resources are mostly automatically created
//  (on misc places) first time they are needed
void CreateD3DResources() { CreateD3dPoolDefaultResources(); }

// to be called before d3d shutdown
void ReleaseD3DResources()
{
  ReleaseD3DPoolDefaultResources();
  if (g_app && g_app->m_location && g_app->m_location->m_landscape.m_renderer)
    g_app->m_location->m_landscape.m_renderer->ReleaseD3DResources();
}

bool Direct3DInit(HWND _hWnd, bool _windowed, int _width, int _height, int _colourDepth, int _zDepth, bool _waitVRT)
{
  // Create the D3D object.
  if (!g_pD3D)
  {
    if (nullptr == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
      // fatal end, no res change can help
      ASSERT_TEXT(false, "Direct3D 9 not available.");
    }
  }

  // Detect hw vertex processing support.
  D3DCAPS9 caps;
  if (SUCCEEDED(g_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,&caps)))
  {
    g_supportsHwVertexProcessing = caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT;
    g_supportsAutoMipmapping = caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP;
  }
  // disable it for testing
  //g_supportsHwVertexProcessing = 0;

  // Set up the structure used to create the D3DDevice. Since we are now
  // using more complex geometry, we will create a device with a zbuffer.
  ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));

  // Default setting for the back buffer format
  g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
  D3DFORMAT displayFormat = D3DFMT_UNKNOWN;

  /*/ Triple buffering, turn off in safe mode
  if(_waitVRT && // makes sense only when waiting for retrace
    (_width>640 || _height>480 || !_windowed) // let's try it off in safe mode to save memory
    )
  {
    // - on slow computer, could make latency higher
    // + could prevent some jerkiness (but not all)
    // + on slow computer, could make fps higher
    g_d3dpp.BackBufferCount = 2;
  }
  */

  if (_windowed)
    g_d3dpp.Windowed = TRUE;
  else
  {
    g_d3dpp.Windowed = FALSE;
    g_d3dpp.BackBufferWidth = _width;
    g_d3dpp.BackBufferHeight = _height;
    switch (_colourDepth)
    {
      case 24:
      case 32:
        g_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
        displayFormat = D3DFMT_X8R8G8B8;
        break;
      case 15:
      case 16:
        g_d3dpp.BackBufferFormat = D3DFMT_X1R5G5B5;
        displayFormat = D3DFMT_X1R5G5B5;
        break;
      default: ASSERT_TEXT(false, "Unexpected colourDepth.");
    }
  }

  g_d3dpp.PresentationInterval = _waitVRT ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
  g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

  if (_zDepth)
  {
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    switch (_zDepth)
    {
      case 16:
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;//D3DFMT_D15S1
        break;
      case 24:
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;//D3DFMT_D24S8;D3DFMT_D24FS8;D3DFMT_D24X4S4;
        break;
      case 32:
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
        break;
      default: ASSERT_TEXT(false, "Unexpected zDepth.");
    }
  }
  g_d3dpp.hDeviceWindow = _hWnd;

  // Adjust format.
  if (!_windowed)
  {
    HRESULT hr = g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, displayFormat, g_d3dpp.BackBufferFormat,
                                         g_d3dpp.Windowed);
    if (FAILED(hr))
    {
      if (_colourDepth == 15 || _colourDepth == 16)
      {
        // Fixes 16bit modes on Radeon X300
        g_d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
        displayFormat = D3DFMT_R5G6B5;
        HRESULT hr = g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, displayFormat, g_d3dpp.BackBufferFormat,
                                             g_d3dpp.Windowed);
        if (FAILED(hr))
        {
          // last existing case
          g_d3dpp.BackBufferFormat = D3DFMT_A1R5G5B5;
          displayFormat = D3DFMT_X1R5G5B5;
        }
      }
      else
      {
        // last existing case
        g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
        displayFormat = D3DFMT_X8R8G8B8;
      }
    }
    hr = g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, displayFormat, g_d3dpp.BackBufferFormat, g_d3dpp.Windowed);
    DEBUG_ASSERT(SUCCEEDED(hr));
    if (!SUCCEEDED(hr))
    {
      // when all checks failed, return back to default mode and hope that something is wrong _only with checks_
      g_d3dpp.BackBufferFormat = (_colourDepth >= 24) ? D3DFMT_X8R8G8B8 : D3DFMT_X1R5G5B5;
    }
  }

  // Create the D3DDevice
  if (FAILED(g_pD3D->CreateDevice(
    // NVPerfHud adapter
    //g_pD3D->GetAdapterCount()-1, D3DDEVTYPE_REF,
    // standard adapter
    D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, _hWnd, g_supportsHwVertexProcessing?D3DCREATE_HARDWARE_VERTEXPROCESSING:
    D3DCREATE_SOFTWARE_VERTEXPROCESSING,&g_d3dpp, &g_pd3dDevice )))
  {
    // non fatal end, res change can help
    //ASSERT_TEXT( 0, "Failed to set screen mode" );
    return false;
  }

  g_pd3dDeviceActual = g_pd3dDevice;

  InitialiseData();

  g_pd3dDevice->BeginScene();
  g_pd3dDevice->SetVertexDeclaration(s_pCustomVertexDecl);
  // g_pd3dDevice->SetFVF(CustomVertexFVF);

  CreateD3DResources();

  // setup for specular
  g_pd3dDevice->SetRenderState(D3DRS_LOCALVIEWER, FALSE);

  return true;
}

void Direct3DShutdown()
{
  ReleaseD3DResources();
  SAFE_RELEASE(g_pd3dDevice);
  SAFE_RELEASE(g_pD3D);
}

void WaitAndResetDevice()
{
  ReleaseD3DPoolDefaultResources();

  // wait until device can be reset and reset device
  while (true)
  {
    Sleep(100);

    HRESULT hr;
    if (FAILED(hr = g_pd3dDevice->TestCooperativeLevel()))
    {
      if (hr == D3DERR_DEVICENOTRESET)
      {
        hr = g_pd3dDeviceActual->Reset(&g_d3dpp);
        if (FAILED(hr))
          continue;
        break;
      }
    }

    g_inputManager->PollForEvents();
  }

  Resource::FlushOpenGlState();
  CreateD3dPoolDefaultResources();
  Resource::RegenerateOpenGlState();
  g_pd3dDevice->SetVertexDeclaration(s_pCustomVertexDecl);
}

void Direct3DSwapBuffers()
{
  g_pd3dDevice->EndScene();
  if (g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr) == D3DERR_DEVICELOST)
    WaitAndResetDevice();
  g_pd3dDevice->BeginScene();

#ifdef FRAMES_PER_SECOND_COUNTER
  static DWORD s_startTime = GetTickCount();
  static unsigned s_numFrames = 0;

  s_numFrames++;

  if (GetTickCount() - s_startTime > 5000)
  {
    s_numFrames = 0;
    s_startTime = GetTickCount();
  }
#endif // FRAMES_PER_SECOND_COUNTER
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
  s_clearColor = D3DCOLOR_COLORVALUE(red, green, blue, alpha);
}

void glClear(GLbitfield mask)
{
  DWORD flags = 0;

  if (mask & GL_COLOR_BUFFER_BIT)
    flags |= D3DCLEAR_TARGET;

  if (mask & GL_DEPTH_BUFFER_BIT)
    flags |= D3DCLEAR_ZBUFFER;

  // Not implemented:
  // OpenGL supports specifying which buffers to clear
  // (Front or Back buffers, left or right stereoscopic)
  // by means of glDrawBuffers

  g_pd3dDevice->Clear(0, nullptr, flags, s_clearColor, 1.0f, 0);

  // Ensure that mask does not include any flags that we may not
  // have handled.
  DEBUG_ASSERT((mask & ~(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)) == 0);
}

void glMatrixMode(GLenum mode)
{
  DEBUG_ASSERT(mode == GL_MODELVIEW || mode == GL_PROJECTION);

  // Nothing to be done if we're already in this matrix mode
  if (s_matrixMode == mode)
    return;

  s_pTargetMatrixStack->FastSetTransform();

  s_matrixMode = mode;
  switch (mode)
  {
    case GL_MODELVIEW:
      s_pTargetMatrixStack = s_pModelViewMatrixStack;
      s_targetMatrixTransformType = D3DTS_WORLD;
      break;

    case GL_PROJECTION:
      s_pTargetMatrixStack = s_pProjectionMatrixStack;
      s_targetMatrixTransformType = D3DTS_PROJECTION;
      break;
  }
}

void glLoadIdentity()
{
  D3DXMATRIX m;
  D3DXMatrixIdentity(&m);
  s_pTargetMatrixStack->Load(m);
}

void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
  D3DXMATRIX m;
  D3DXMatrixOrthoOffCenterRH(&m, left, right, bottom, top, -1.0, +1.0);
  s_pTargetMatrixStack->Load(m);
}

void gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx,
               GLdouble upy, GLdouble upz)
{
  D3DXVECTOR3 eye(static_cast<FLOAT>(eyex), static_cast<FLOAT>(eyey), static_cast<FLOAT>(eyez)), at(
                static_cast<FLOAT>(centerx), static_cast<FLOAT>(centery), static_cast<FLOAT>(centerz)), up(
                static_cast<FLOAT>(upx), static_cast<FLOAT>(upy), static_cast<FLOAT>(upz));

  D3DXMATRIX m;
  D3DXMatrixLookAtRH(&m, &eye, &at, &up);
  s_pTargetMatrixStack->Load(m);
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
  D3DXMATRIX m;
  D3DXMatrixPerspectiveFovRH(&m, D3DXToRadian(static_cast<FLOAT>(fovy)), static_cast<FLOAT>(aspect), static_cast<FLOAT>(zNear),
                             static_cast<FLOAT>(zFar));
  // Preserve OpenGL compatibility
  m._43 *= 2.0f;
  s_pTargetMatrixStack->Load(m);
}

static void setupTexturing()
{
  // JAK: I think that the glPushAttrib stuff might be interfering with
  //      the commented optimisation below:

  //if (s.envMode != s.lastEnvMode ||
  //	s.target != s.lastTarget) {

  TextureState& s = *s_activeTextureState;

  s.lastEnvMode = s.envMode;
  s.lastTarget = s.target;

  g_pd3dDevice->SetTexture(s_activeTexture, s_textureIds[s.target]);

  // See table 9-15 in Red Book (page 418) for description of texture env modes.
  // We assume that all textures are in RGBA format.

  switch (s.envMode)
  {
    case GL_MODULATE:
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_MODULATE);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
      break;

    case GL_REPLACE:
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTA_TEXTURE);

      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      break;

    case GL_DECAL:
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_MODULATE);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
      break;

    case GL_COMBINE_EXT:
      switch (s.rgbCombineMode)
      {
        case GL_REPLACE:
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTA_TEXTURE);

          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

          break;

        case GL_MULT:
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_MODULATE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG2, D3DTA_CURRENT);

          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
          break;

        case GL_MODULATE:
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_MODULATE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLORARG2, D3DTA_CURRENT);

          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
          g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
          break;
      }
  }
  //}
}

void __stdcall OpenGLD3D::glActiveTextureD3D(int _target)
{
  // Multi-texturing

  _target -= GL_TEXTURE0_ARB;
  DEBUG_ASSERT(_target >= 0 && _target < MAX_ACTIVE_TEXTURES);

  s_activeTexture = _target;
  s_activeTextureState = &s_textureStates[s_activeTexture];
}

void __stdcall OpenGLD3D::glMultiTexCoord2fD3D(int _target, float _x, float _y)
{
  _target -= GL_TEXTURE0_ARB;
  DEBUG_ASSERT(_target >= 0 && _target < MAX_ACTIVE_TEXTURES);

  if (_target == 0)
  {
    s_currentAttribs.u = _x;
    s_currentAttribs.v = _y;
  }
  else
  {
    s_currentAttribs.u2 = _x;
    s_currentAttribs.v2 = _y;
  }
}

static void setupColorMaterial()
{
  if (s_currentAttribs.colorMaterialEnabled)
  {
    switch (s_currentAttribs.colorMaterialMode)
    {
      case GL_AMBIENT_AND_DIFFUSE:
        g_pd3dDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
        g_pd3dDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
        g_pd3dDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
        break;

      case GL_DIFFUSE:
        g_pd3dDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
        g_pd3dDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
        g_pd3dDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
        break;

      default: DEBUG_ASSERT(FALSE);
    }
  }
  else
  {
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
    g_pd3dDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    g_pd3dDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
  }
}

void glDisable(GLenum cap)
{
  switch (cap)
  {
    case GL_ALPHA_TEST:
      g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
      break;

    case GL_BLEND:
      g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      break;

    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    {
      DWORD clipPlanes;
      g_pd3dDevice->GetRenderState(D3DRS_CLIPPLANEENABLE, &clipPlanes);
      g_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, clipPlanes & ~(1 << (cap - GL_CLIP_PLANE0)));
      break;
    }

    case GL_COLOR_MATERIAL:
      s_currentAttribs.colorMaterialEnabled = false;
      setupColorMaterial();
      break;

    case GL_CULL_FACE:
      s_currentAttribs.backCullingEnabled = false;
      g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
      break;

    case GL_DEPTH_TEST:
      g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
      break;

    case GL_FOG:
      g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
      break;

    case GL_LIGHTING:
      g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
      break;

    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
      g_pd3dDevice->LightEnable(cap - GL_LIGHT0, FALSE);
      break;

    case GL_LINE_SMOOTH:
      g_pd3dDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
      break;

    case GL_NORMALIZE:
      g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
      break;

    case GL_SCISSOR_TEST:
      g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
      break;

    case GL_POINT_SMOOTH:
      // Don't think Direct3D supports smooth (circular) points anyway
      break;

    case GL_TEXTURE_2D:
      s_currentAttribs.texturingEnabled[s_activeTexture] = false;
      g_pd3dDevice->SetTextureStageState(s_activeTexture, D3DTSS_COLOROP, D3DTOP_DISABLE);
      break;
  }
}

void glEnable(GLenum cap)
{
  switch (cap)
  {
    case GL_ALPHA_TEST:
      g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
      break;

    case GL_BLEND:
      g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      break;

    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    {
      DWORD clipPlanes;
      g_pd3dDevice->GetRenderState(D3DRS_CLIPPLANEENABLE, &clipPlanes);
      g_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, clipPlanes | (1 << (cap - GL_CLIP_PLANE0)));
      break;
    }

    case GL_COLOR_MATERIAL:
      s_currentAttribs.colorMaterialEnabled = true;
      setupColorMaterial();
      break;

    case GL_CULL_FACE:
      s_currentAttribs.backCullingEnabled = true;
      g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, s_cullMode);
      break;

    case GL_DEPTH_TEST:
      g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
      break;

    case GL_FOG:
      g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
      break;

    case GL_LIGHTING:
      g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
      break;

    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
      g_pd3dDevice->LightEnable(cap - GL_LIGHT0, TRUE);
      break;

    case GL_LINE_SMOOTH:
      g_pd3dDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
      break;

    case GL_NORMALIZE:
      g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
      break;

    case GL_SCISSOR_TEST:
      g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
      break;

    case GL_TEXTURE_2D:
      if (s_textureIds[s_activeTextureState->target])
        setupTexturing();

      s_currentAttribs.texturingEnabled[s_activeTexture] = true;
      break;
  }
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
  s_currentAttribs.r8 = red;
  s_currentAttribs.g8 = green;
  s_currentAttribs.b8 = blue;
  s_currentAttribs.a8 = alpha;
}

void glNormal3f_impl(GLfloat nx, GLfloat ny, GLfloat nz)
{
  // Direct3D Normals have to be inverted due to the left handed coordinate system.
  s_currentAttribs.nx = -nx;
  s_currentAttribs.ny = -ny;
  s_currentAttribs.nz = -nz;
}

static void reallocateVertexList()
{
  // Expand the vertex buffer
  unsigned allocatedVertices = s_allocatedVertices * 2;
  auto buffer = NEW CustomVertex[allocatedVertices];

  memcpy(buffer, s_vertices, sizeof(CustomVertex) * s_currentVertexNumber);
  delete[] s_vertices;

  s_vertices = buffer;
  s_allocatedVertices = allocatedVertices;
  s_currentVertex = s_vertices + s_currentVertexNumber;
}

void glBegin(GLenum mode)
{
  // glBegin is used to draw primitives
  // Note: opportunity for object orientation here
  //		 (Refactoring switch statements in glBegin, glEnd)

  s_currentVertex = s_vertices;
  s_currentVertexNumber = 0;
  s_primitiveMode = mode;
}

void glEnd()
{
  // Update the matrix if it has been modified (also updated when we switch matrix modes)
  s_pTargetMatrixStack->FastSetTransform();

  switch (s_primitiveMode)
  {
    case GL_POINTS: DEBUG_ASSERT(s_currentVertexNumber>0);
      g_pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, s_currentVertexNumber, s_vertices, sizeof(CustomVertex));
      break;

    case GL_LINES: DEBUG_ASSERT(s_currentVertexNumber%2==0);
      DEBUG_ASSERT(s_currentVertexNumber>1);
      g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINELIST, s_currentVertexNumber / 2, s_vertices, sizeof(CustomVertex));
      break;

    case GL_LINE_LOOP: DEBUG_ASSERT(s_currentVertexNumber>1);
      if (s_currentVertexNumber + 2 >= s_allocatedVertices)
        reallocateVertexList();
      s_vertices[s_currentVertexNumber] = s_vertices[0];
      g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, s_currentVertexNumber, s_vertices, sizeof(CustomVertex));
      break;

    case GL_TRIANGLES:
      // Reverse the triangle vertices
      DEBUG_ASSERT(s_currentVertexNumber%3==0);
      DEBUG_ASSERT(s_currentVertexNumber>2);
      g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, s_currentVertexNumber / 3, s_vertices, sizeof(CustomVertex));
      break;

    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP: DEBUG_ASSERT(s_currentVertexNumber>2);
      if (s_currentVertexNumber > 2) // invalid value here _could be_ cause of xander's problem
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, s_currentVertexNumber - 2, s_vertices, sizeof(CustomVertex));
      break;

    case GL_TRIANGLE_FAN: DEBUG_ASSERT(s_currentVertexNumber>2);
      g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, s_currentVertexNumber - 2, s_vertices, sizeof(CustomVertex));
      break;

    case GL_QUADS:
      // Inefficient, perhaps we should have two buffers, a vertex list, and an index list
      // When adding a vertex using glVertex, we could add just the vertex to the vertex list
      // and then add the appropriate indexes depending on the primitive mode.
      // Would require using proper vertex buffers though.
    {
      DEBUG_ASSERT(s_currentVertexNumber%3==0);
      int numTris = s_currentVertexNumber / 3;
      // could be 0 in PaintPixels, and it's difficult to filter it out sooner
      if (numTris)
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, numTris, s_vertices, sizeof(CustomVertex));
    }
    break;
  }
}

void glVertex3f_impl(GLfloat x, GLfloat y, GLfloat z)
{
  // Check to see if we need to expand the list
  if (s_currentVertexNumber + 2 >= s_allocatedVertices)
    reallocateVertexList();

  *s_currentVertex = s_currentAttribs;
  s_currentVertex->x = x;
  s_currentVertex->y = y;
  s_currentVertex->z = z;

  s_currentVertex++;
  s_currentVertexNumber++;

  if (s_primitiveMode == GL_QUADS)
  {
    static int vertexCount = 0;

    if (vertexCount == 2)
    {
      // Add in some more vertices to make up the quad

      *s_currentVertex = s_vertices[s_currentVertexNumber - 3];
      s_currentVertex++;
      s_currentVertexNumber++;

      *s_currentVertex = s_vertices[s_currentVertexNumber - 2];
      s_currentVertex++;
      s_currentVertexNumber++;
    }

    vertexCount = (vertexCount + 1) & 3;
  }
}

void glGenTextures(GLsizei n, GLuint* textures)
{
  // We start off with a NULL pointer for the texture
  // This will be replaced with an actual pointer
  // by a call to glTexImage2D (or gluBuild2DMipmaps)

  for (int i = 0; i < n; i++)
  {
    textures[i] = s_textureIds.size();
    s_textureIds.push_back(nullptr);
  }
}

GLuint glGenTextures(LPDIRECT3DTEXTURE9 _texture)
{
  const auto val = s_textureIds.size();
  s_textureIds.emplace_back(_texture);
  return static_cast<GLuint>(val);
}

void glBindTexture(GLenum target, GLuint texture)
{
  switch (target)
  {
    case GL_TEXTURE_2D:
      s_activeTextureState->target = texture;
      if (s_currentAttribs.texturingEnabled[s_activeTexture])
        setupTexturing();
      break;

    default: DEBUG_ASSERT(target == GL_TEXTURE_2D);
      break;
  }
}

// This function is basically the same as glTexImage2D, except that this function is
// supposed to produce mipmapped textures

int gluBuild2DMipmaps(GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void* data)
{
  DEBUG_ASSERT(target == GL_TEXTURE_2D);

  LPDIRECT3DTEXTURE9& pTexture = s_textureIds[s_activeTextureState->target];

  // Free the previous texture if necessary
  if (pTexture != nullptr)
  {
    pTexture->Release();
    pTexture = nullptr;
  }

  // We enforce that the width and height are powers of 2.
  // gluBuild2DMipmaps ought to be able to deal with it, but
  // it's easier to get the application to do it.
  DEBUG_ASSERT(width == 1 << static_cast<int>(ceil(log((float)width) / log(2.0f))));
  DEBUG_ASSERT(height == 1 << static_cast<int>(ceil(log((float)height) / log(2.0f))));

  // We also enforce that the format is convenient
  DEBUG_ASSERT(format == GL_RGBA);
  DEBUG_ASSERT(type == GL_UNSIGNED_BYTE);

  HRESULT rc = g_pd3dDevice->CreateTexture(width, height, 0, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture,
                                           nullptr);
  DEBUG_ASSERT(rc != D3DERR_INVALIDCALL);
  DEBUG_ASSERT(rc != D3DXERR_INVALIDDATA);

  IDirect3DSurface9* pSurface;
  rc = pTexture->GetSurfaceLevel(0, &pSurface);

  RECT rect = {0, 0, width, height};

  // Triangle filtering is the default, but it is also the slowest filter
  // Box filtering ought to work ok for us.
  rc = D3DXLoadSurfaceFromMemory(pSurface, nullptr, nullptr, data, D3DFMT_A8B8G8R8, width * 4, nullptr, &rect,
                                 D3DX_FILTER_NONE /* D3DX_FILTER_BOX */, 0);

  DEBUG_ASSERT(rc != D3DERR_INVALIDCALL);
  DEBUG_ASSERT(rc != D3DXERR_INVALIDDATA);

  pSurface->Release();

  pTexture->GenerateMipSubLevels();

  if (s_currentAttribs.texturingEnabled)
    setupTexturing();

  return 0;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format,
                  GLenum type, const GLvoid* pixels)
{
  DEBUG_ASSERT(target == GL_TEXTURE_2D);
  DEBUG_ASSERT(level == 0);
  DEBUG_ASSERT(internalformat == 4);
  DEBUG_ASSERT(format == GL_RGBA);

  gluBuild2DMipmaps(target, 4, width, height, format, type, pixels);
}

void glTexCoord2f_impl(GLfloat u, GLfloat v)
{
  s_currentAttribs.u = u;
  s_currentAttribs.v = v;
}

static DWORD glMinMagFilterToDirect3D(GLenum param)
{
  switch (param)
  {
    case GL_LINEAR:
      return D3DTEXF_LINEAR;
    case GL_NEAREST:
      return D3DTEXF_POINT;
    case GL_LINEAR_MIPMAP_LINEAR:
      return D3DTEXF_LINEAR;
    default: ASSERT_TEXT(FALSE, "Unknown Filter %u", param);
      return D3DTEXF_NONE;
  }
}

static GLenum direct3DMinMagFilterToOpenGL(DWORD param)
{
  switch (param)
  {
    case D3DTEXF_LINEAR:
      return GL_LINEAR;
    case D3DTEXF_POINT:
      return GL_NEAREST;
    default: ASSERT_TEXT(FALSE, "Unknown Filter %u", param);
      return 0;
  }
}

static DWORD glTextureWrapToDirect3D(GLenum param)
{
  switch (param)
  {
    case GL_CLAMP:
      return D3DTADDRESS_CLAMP;
    case GL_REPEAT:
      return D3DTADDRESS_WRAP;
    default: ASSERT_TEXT(FALSE, "Unknown LegacyTexture Wrap mode %u", param);
      return D3DTADDRESS_WRAP;
  }
}

// Implement glTexParameteri , glTexParameterf
//  See: SetSamplerState and 'LegacyTexture Wrapping' and 'LegacyTexture Addressing Modes'

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
  DEBUG_ASSERT(target == GL_TEXTURE_2D);

  switch (pname)
  {
    case GL_TEXTURE_WRAP_S:
      g_pd3dDevice->SetSamplerState(s_activeTexture, D3DSAMP_ADDRESSU, glTextureWrapToDirect3D(param));
      break;

    case GL_TEXTURE_WRAP_T:
      g_pd3dDevice->SetSamplerState(s_activeTexture, D3DSAMP_ADDRESSV, glTextureWrapToDirect3D(param));
      break;

    case GL_TEXTURE_MAG_FILTER:
      g_pd3dDevice->SetSamplerState(s_activeTexture, D3DSAMP_MAGFILTER, glMinMagFilterToDirect3D(param));
      break;

    case GL_TEXTURE_MIN_FILTER:
      g_pd3dDevice->SetSamplerState(s_activeTexture, D3DSAMP_MINFILTER, glMinMagFilterToDirect3D(param));
      g_pd3dDevice->SetSamplerState(s_activeTexture, D3DSAMP_MIPFILTER,
                                    param == GL_LINEAR_MIPMAP_LINEAR ? D3DTEXF_LINEAR : D3DTEXF_NONE);
      break;
  }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
  glTexParameteri(target, pname, param);
}

static D3DBLEND openGLBlendFactorToDirect3D(GLenum factor)
{
  switch (factor)
  {
    case GL_ZERO:
      return D3DBLEND_ZERO;
    case GL_ONE:
      return D3DBLEND_ONE;
    case GL_SRC_COLOR:
      return D3DBLEND_SRCCOLOR;
    case GL_ONE_MINUS_SRC_COLOR:
      return D3DBLEND_INVSRCCOLOR;
    case GL_SRC_ALPHA:
      return D3DBLEND_SRCALPHA;
    case GL_ONE_MINUS_SRC_ALPHA:
      return D3DBLEND_INVSRCALPHA;

    default:
      // Unknown blending factor
      DEBUG_ASSERT(FALSE);
      return D3DBLEND_ONE;
  }
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
  g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, openGLBlendFactorToDirect3D(sfactor));
  g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, openGLBlendFactorToDirect3D(dfactor));
}

void glDepthMask(GLboolean flag)
{
  g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, flag);
}

void glFogf_impl(GLenum pname, GLfloat param)
{
  switch (pname)
  {
    case GL_FOG_DENSITY:
      g_pd3dDevice->SetRenderState(D3DRS_FOGDENSITY, *((DWORD*)(&param)));
      break;

    case GL_FOG_END:
      g_pd3dDevice->SetRenderState(D3DRS_FOGEND, *((DWORD*)(&param)));
      break;

    case GL_FOG_MODE:
      glFogi(GL_FOG_MODE, *((GLint*)(&param)));
      break;

    case GL_FOG_START:
      g_pd3dDevice->SetRenderState(D3DRS_FOGSTART, *((DWORD*)(&param)));
      break;

    default:
      // Invalid fog parameter (colour must be specified using the 'v' variant).
      DEBUG_ASSERT(FALSE);
      break;
  }
}

static D3DFOGMODE openGLFogModeToDirect3D(GLenum param)
{
  switch (param)
  {
    case GL_LINEAR:
      return D3DFOG_LINEAR;
    //case GL_EXP: return D3DFOG_EXP;
    //case GL_EXP2: return D3DFOG_EXP2;
    default:
      // Invalid fog mode
      DEBUG_ASSERT(FALSE);
      return D3DFOG_NONE;
  }
}

void glFogi_impl(GLenum pname, GLint param)
{
  switch (pname)
  {
    case GL_FOG_DENSITY:
    case GL_FOG_END:
    case GL_FOG_START:
      if (param > 0)
        glFogf_impl(pname, static_cast<float>(param) / INT_MAX);
      else
        glFogf_impl(pname, static_cast<float>(param) / INT_MIN);
      break;

    case GL_FOG_MODE:
      // glHint( GL_FOG, GL_DONT_CARE ) should result in D3DRS_FOGVERTEXMODE being used
      // instead, if pixel based fog is inefficient

      // g_pd3dDevice->SetRenderState( D3DRS_FOGVERTEXMODE, openGLFogModeToDirect3D((GLenum) param) );
      g_pd3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, openGLFogModeToDirect3D(static_cast<GLenum>(param)));
      break;
  }
}

void glFogfv(GLenum pname, const GLfloat* params)
{
  switch (pname)
  {
    case GL_FOG_COLOR:
      g_pd3dDevice->SetRenderState(D3DRS_FOGCOLOR, D3DCOLOR_COLORVALUE(params[0], params[1], params[2], params[3]));
      break;

    default:
      // Not implemented
      DEBUG_ASSERT(FALSE);
      break;
  }
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  D3DVIEWPORT9 viewport;

  g_pd3dDevice->GetViewport(&viewport);
  viewport.X = x;
  viewport.Y = y;
  viewport.Width = width;
  viewport.Height = height;

  g_pd3dDevice->SetViewport(&viewport);
}

static GLenum direct3DFillModeToOpenGL(D3DFILLMODE mode)
{
  switch (mode)
  {
    case D3DFILL_SOLID:
      return GL_FILL;

    case D3DFILL_WIREFRAME:
      return GL_LINE;

    default: DEBUG_ASSERT(mode == D3DFILL_SOLID || mode == D3DFILL_WIREFRAME);
      return GL_FILL;
  }
}

static GLenum direct3DShadeModelToOpenGL(D3DSHADEMODE mode)
{
  switch (mode)
  {
    case D3DSHADE_FLAT:
      return GL_FLAT;

    case D3DSHADE_PHONG:
    case D3DSHADE_GOURAUD:
      return GL_SMOOTH;

    default: DEBUG_ASSERT(FALSE);
      return GL_SMOOTH;
  }
}

static GLenum direct3DFuncToOpenGL(D3DCMPFUNC func)
{
  switch (func)
  {
    case D3DCMP_LESSEQUAL:
      return GL_LEQUAL;
    case D3DCMP_GREATER:
      return GL_GREATER;
    default: DEBUG_ASSERT(FALSE);
      return GL_LEQUAL;
  }
}

static GLenum direct3DBlendFactorToOpenGL(D3DBLEND factor)
{
  switch (factor)
  {
    case D3DBLEND_ZERO:
      return GL_ZERO;
    case D3DBLEND_ONE:
      return GL_ONE;
    case D3DBLEND_SRCCOLOR:
      return GL_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
      return GL_ONE_MINUS_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
      return GL_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
      return GL_ONE_MINUS_SRC_ALPHA;

    default:
      // Unknown blending factor
      DEBUG_ASSERT(FALSE);
      return GL_ONE;
  }
}

static GLenum direct3DFogModeToOpenGL(D3DFOGMODE param)
{
  switch (param)
  {
    case D3DFOG_LINEAR:
      return GL_LINEAR;
    //case GL_EXP: return D3DFOG_EXP;
    //case GL_EXP2: return D3DFOG_EXP2;
    default:
      // Invalid fog mode
      DEBUG_ASSERT(FALSE);
      return GL_LINEAR;
  }
}

void glGetIntegerv(GLenum pname, GLint* params)
{
  switch (pname)
  {
    case GL_ALPHA_TEST_FUNC:
    {
      D3DCMPFUNC func;
      g_pd3dDevice->GetRenderState(D3DRS_ALPHAFUNC, (DWORD*)&func);
      params[0] = direct3DFuncToOpenGL(func);
      break;
    }

    case GL_BLEND_DST:
    {
      D3DBLEND factor;
      g_pd3dDevice->GetRenderState(D3DRS_DESTBLEND, (DWORD*)&factor);
      params[0] = direct3DBlendFactorToOpenGL(factor);
      break;
    }

    case GL_BLEND_SRC:
    {
      D3DBLEND factor;
      g_pd3dDevice->GetRenderState(D3DRS_SRCBLEND, (DWORD*)&factor);
      params[0] = direct3DBlendFactorToOpenGL(factor);
      break;
    }

    case GL_DEPTH_FUNC:
    {
      D3DCMPFUNC func;
      g_pd3dDevice->GetRenderState(D3DRS_ZFUNC, (DWORD*)&func);
      params[0] = direct3DFuncToOpenGL(func);
      break;
    }

    case GL_DEPTH_WRITEMASK:
    {
      DWORD state;
      g_pd3dDevice->GetRenderState(D3DRS_ZWRITEENABLE, &state);
      params[0] = state == TRUE;
      break;
    }

    case GL_COLOR_MATERIAL_FACE:
      // Direct3D doesn't seem to support independent front and back materials
      params[0] = GL_FRONT_AND_BACK;
      break;

    case GL_COLOR_MATERIAL_PARAMETER:
      params[0] = s_currentAttribs.colorMaterialMode;
      break;

      case GL_FOG_MODE:
    {
      D3DFOGMODE mode;
      g_pd3dDevice->GetRenderState(D3DRS_FOGTABLEMODE, (DWORD*)&mode);
      params[0] = direct3DFogModeToOpenGL(mode);
      break;
    }

    case GL_FRONT_FACE:
      params[0] = s_ccwFrontFace ? GL_CCW : GL_CW;
      break;

    case GL_MATRIX_MODE:
      params[0] = s_matrixMode;
      break;

    case GL_POLYGON_MODE:
    {
      D3DFILLMODE mode;
      g_pd3dDevice->GetRenderState(D3DRS_FILLMODE, (DWORD*)&mode);
      params[0] = params[1] = direct3DFillModeToOpenGL(mode);
      break;
    }

    case GL_SHADE_MODEL:
    {
      D3DSHADEMODE mode;
      g_pd3dDevice->GetRenderState(D3DRS_SHADEMODE, (DWORD*)&mode);
      params[0] = direct3DShadeModelToOpenGL(mode);
      break;
    }
    case GL_VIEWPORT:
    {
      D3DVIEWPORT9 viewport;
      g_pd3dDevice->GetViewport(&viewport);

      params[0] = viewport.X;
      params[1] = viewport.Y;
      params[2] = viewport.Width;
      params[3] = viewport.Height;
    }
    break;
  }
}

static void direct3DColorToFloatv(D3DCOLOR colour, GLfloat* params)
{
  // ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
  params[1] = ((colour >> 16) & 0xff) / 255.0f;
  params[2] = ((colour >> 8) & 0xff) / 255.0f;
  params[3] = ((colour) & 0xff) / 255.0f;
  params[4] = ((colour >> 24) & 0xff) / 255.0f;
}

void glGetFloatv(GLenum pname, GLfloat* params)
{
  switch (pname)
  {
    case GL_ALPHA_TEST_REF:
      params[0] = s_currentAttribs.alphaRef;
      break;

    case GL_CURRENT_COLOR:
      params[0] = s_currentAttribs.r8 * 0.003921568627450980392156862745098f;
      params[1] = s_currentAttribs.g8 * 0.003921568627450980392156862745098f;
      params[2] = s_currentAttribs.b8 * 0.003921568627450980392156862745098f;
      params[3] = s_currentAttribs.a8 * 0.003921568627450980392156862745098f;
      break;

    case GL_FOG_COLOR:
    {
      D3DCOLOR colour;
      g_pd3dDevice->GetRenderState(D3DRS_FOGCOLOR, &colour);
      direct3DColorToFloatv(colour, params);
    }
    break;

    case GL_FOG_DENSITY:
      g_pd3dDevice->GetRenderState(D3DRS_FOGDENSITY, (DWORD*)params);
      break;

    case GL_FOG_END:
      g_pd3dDevice->GetRenderState(D3DRS_FOGEND, (DWORD*)params);
      break;

    case GL_LIGHT_MODEL_AMBIENT:
    {
      D3DCOLOR colour;
      g_pd3dDevice->GetRenderState(D3DRS_AMBIENT, &colour);
      direct3DColorToFloatv(colour, params);
    }
    break;

    default: 
      DEBUG_ASSERT(false);
      break;
  }
}

template <class DoubleFloat>
static void direct3DMatrixToOpenGL(const D3DMATRIX& _matrix, DoubleFloat* _out)
{
  // OpenGL matrices are in column-major order, whereas Direct3D matrices
  // are in row-major order.
  //
  // However, Direct3D Matrices are transposed already, because Direct3D does pre-multiplying
  // and OpenGL does post-multiplying. Therefore, we do not need to transpose them
  // again.

  // We should provide a specialisation of this function which performs a
  // memcpy for Float.
  _out[0] = _matrix._11;
  _out[1] = _matrix._12;
  _out[2] = _matrix._13;
  _out[3] = _matrix._14;

  _out[4] = _matrix._21;
  _out[5] = _matrix._22;
  _out[6] = _matrix._23;
  _out[7] = _matrix._24;

  _out[8] = _matrix._31;
  _out[9] = _matrix._32;
  _out[10] = _matrix._33;
  _out[11] = _matrix._34;

  _out[12] = _matrix._41;
  _out[13] = _matrix._42;
  _out[14] = _matrix._43;
  _out[15] = _matrix._44;
}

template <class DoubleFloat>
static void openGLMatrixToDirect3D(const DoubleFloat* _in, D3DMATRIX& _out)
{
  // See comment in direct3DMatrixToOpenGL

  _out._11 = _in[0];
  _out._12 = _in[1];
  _out._13 = _in[2];
  _out._14 = _in[3];

  _out._21 = _in[4];
  _out._22 = _in[5];
  _out._23 = _in[6];
  _out._24 = _in[7];

  _out._31 = _in[8];
  _out._32 = _in[9];
  _out._33 = _in[10];
  _out._34 = _in[11];

  _out._41 = _in[12];
  _out._42 = _in[13];
  _out._43 = _in[14];
  _out._44 = _in[15];
}

void glGetDoublev(GLenum pname, float* params)
{
  switch (pname)
  {
    case GL_MODELVIEW_MATRIX:
    {
      D3DXMATRIX matrix = s_pModelViewMatrixStack->GetTransform();
      direct3DMatrixToOpenGL(matrix, params);
    }
    break;

    case GL_PROJECTION_MATRIX:
    {
      D3DXMATRIX matrix = s_pProjectionMatrixStack->GetTransform();
      direct3DMatrixToOpenGL(matrix, params);
    }
    break;
  }
}

int gluProject(GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16],
               const GLint viewport[4], GLdouble* winx, GLdouble* winy, GLdouble* winz)
{
  D3DXMATRIX model, proj;

  openGLMatrixToDirect3D(modelMatrix, model);
  openGLMatrixToDirect3D(projMatrix, proj);

  D3DXVECTOR4 obj(objx, objy, objz, 1.0f);
  D3DXVECTOR4 win0, win;
  D3DXVec4Transform(&win0, &obj, &model);
  D3DXVec4Transform(&win, &win0, &proj);

  if (win.w == 0.0f)
    return false;

  win /= win.w;

  // Map from [-1, 1] to [0, 1]
  win.x = win.x * 0.5f + 0.5f;
  win.y = win.y * 0.5f + 0.5f;
  win.z = win.z * 0.5f + 0.5f;

  // Map to viewport
  *winx = win.x * viewport[2] + viewport[0];
  *winy = win.y * viewport[3] + viewport[1];
  *winz = win.z;

  return true;
}

int gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16],
                 const GLint viewport[4], GLdouble* objx, GLdouble* objy, GLdouble* objz)
{
  // Attempt to determine a 3d coordinate that a given window coordinate (winx, winy)
  // corresponds to, given the winz, and model and projection matrices
  // (as well as the viewport dimensions).

  D3DXMATRIX model, proj;

  openGLMatrixToDirect3D(modelMatrix, model);
  openGLMatrixToDirect3D(projMatrix, proj);

  D3DXMATRIX modelProj = model * proj;

  D3DXMATRIX inv;
  if (D3DXMatrixInverse(&inv, nullptr, &modelProj) == nullptr)
    return false;

  D3DXVECTOR4 win((winx - viewport[0] /* left */) / viewport[2] /* width */,
                  (winy - viewport[1] /* top */) / viewport[3] /* height */, winz, 1.0f);

  // Map from [0, 1] to [-1, 1]
  win.x = win.x * 2 - 1;
  win.y = win.y * 2 - 1;
  win.z = win.z * 2 - 1;

  D3DXVECTOR4 source;
  D3DXVec4Transform(&source, &win, &inv);
  if (source.w == 0.0f)
    return false;

  *objx = source.x / source.w;
  *objy = source.y / source.w;
  *objz = source.z / source.w;

  return true;
}

void glLoadMatrixd(const float* _m)
{
  D3DMATRIX m;
  openGLMatrixToDirect3D(_m, m);
  s_pTargetMatrixStack->Load(m);
}

static void transformByModelView(D3DVECTOR& v, float w /* 0.0f for directional, 1.0f for positional */)
{
  D3DXVECTOR4 in(v.x, v.y, v.z, w);
  D3DXVECTOR4 out;

  const D3DXMATRIX* tmp = &s_pModelViewMatrixStack->GetTransform();
  D3DXVec4Transform(&out, &in, tmp);

  v.x = out.x;
  v.y = out.y;
  v.z = out.z;
}

void glLightfv(GLenum light, GLenum pname, const GLfloat* params)
{
  DWORD lightIndex = light - GL_LIGHT0;
  D3DLIGHT9 lightInfo;

  g_pd3dDevice->GetLight(lightIndex, &lightInfo);

  // Apply the modelview matrix to the parameters to get the
  // lights into eye-space

  switch (pname)
  {
    case GL_POSITION:
      if (params[3] == 0.0f)
      {
        lightInfo.Type = D3DLIGHT_DIRECTIONAL;
        lightInfo.Direction.x = params[0];
        lightInfo.Direction.y = params[1];
        lightInfo.Direction.z = params[2];
        transformByModelView(lightInfo.Direction, 0.0f);
      }
      else
      {
        lightInfo.Type = D3DLIGHT_POINT;
        lightInfo.Position.x = params[0];
        lightInfo.Position.y = params[1];
        lightInfo.Position.z = params[2];

        transformByModelView(lightInfo.Direction, 1.0f);
      }
      break;

    case GL_DIFFUSE:
      lightInfo.Diffuse.r = params[0];
      lightInfo.Diffuse.g = params[1];
      lightInfo.Diffuse.b = params[2];
      lightInfo.Diffuse.a = params[3];
      break;

    case GL_SPECULAR:
      lightInfo.Specular.r = params[0];
      lightInfo.Specular.g = params[1];
      lightInfo.Specular.b = params[2];
      lightInfo.Specular.a = params[3];
      break;

    case GL_AMBIENT:
      lightInfo.Ambient.r = params[0];
      lightInfo.Ambient.g = params[1];
      lightInfo.Ambient.b = params[2];
      lightInfo.Ambient.a = params[3];
      break;
  }

  g_pd3dDevice->SetLight(lightIndex, &lightInfo);
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat* params)
{
  int lightIndex = light - GL_LIGHT0;
  D3DLIGHT9 lightInfo;

  g_pd3dDevice->GetLight(lightIndex, &lightInfo);

  switch (pname)
  {
    case GL_POSITION:
      if (lightInfo.Type == D3DLIGHT_DIRECTIONAL)
      {
        params[0] = lightInfo.Direction.x;
        params[1] = lightInfo.Direction.y;
        params[2] = lightInfo.Direction.z;
        params[3] = 0.0f;
      }
      else
      {
        params[0] = lightInfo.Position.x;
        params[1] = lightInfo.Position.y;
        params[2] = lightInfo.Position.z;
        params[3] = 1.0f;
      }
      break;

    case GL_DIFFUSE:
      params[0] = lightInfo.Diffuse.r;
      params[1] = lightInfo.Diffuse.g;
      params[2] = lightInfo.Diffuse.b;
      params[3] = lightInfo.Diffuse.a;
      break;

    case GL_SPECULAR:
      params[0] = lightInfo.Specular.r;
      params[1] = lightInfo.Specular.g;
      params[2] = lightInfo.Specular.b;
      params[3] = lightInfo.Specular.a;
      break;

    case GL_AMBIENT:
      params[0] = lightInfo.Ambient.r;
      params[1] = lightInfo.Ambient.g;
      params[2] = lightInfo.Ambient.b;
      params[3] = lightInfo.Ambient.a;
      break;

    default: DEBUG_ASSERT(false);
      break;
  }
}

void glLightModelfv(GLenum pname, const GLfloat* params)
{
  switch (pname)
  {
    case GL_LIGHT_MODEL_AMBIENT:
      g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(params[0], params[1], params[2], params[3]));
      break;

    default:
      // Unknown (unimplemented) parameter
      DEBUG_ASSERT(FALSE);
  }
}

void glFrontFace(GLenum mode)
{
  bool v = mode == GL_CCW;

  if (s_ccwFrontFace != v)
  {
    s_ccwFrontFace = v;
    s_cullMode = (v ? D3DCULL_CW : D3DCULL_CCW);

    if (s_currentAttribs.backCullingEnabled)
      g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, s_cullMode);
  }
}

static D3DFILLMODE openGLFillModeToDirect3D(GLenum mode)
{
  switch (mode)
  {
    case GL_FILL:
      return D3DFILL_SOLID;

    case GL_LINE:
      return D3DFILL_WIREFRAME;

    default: DEBUG_ASSERT(mode == D3DFILL_SOLID || mode == D3DFILL_WIREFRAME);
      return D3DFILL_WIREFRAME;
  }
}

void glPolygonMode(GLenum face, GLenum mode)
{
  g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, openGLFillModeToDirect3D(mode));
}

static D3DSHADEMODE openGLShadeModelToDirect3D(GLenum mode)
{
  switch (mode)
  {
    case GL_FLAT:
      return D3DSHADE_FLAT;

    case GL_SMOOTH:
      return D3DSHADE_GOURAUD;

    default: DEBUG_ASSERT(mode == GL_FLAT || mode == GL_SMOOTH);
      return D3DSHADE_FLAT;
  }
}

void glShadeModel(GLenum mode)
{
  g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, openGLShadeModelToDirect3D(mode));
}

void glPushMatrix()
{
  s_pTargetMatrixStack->Push();
}

void glPopMatrix()
{
  s_pTargetMatrixStack->Pop();
}

void glMultMatrixf(const GLfloat* m)
{
  D3DXMATRIX mat;
  openGLMatrixToDirect3D(m, mat);
  s_pTargetMatrixStack->Multiply(mat);
}

// Used by the radar dish for the clip planes (horrid hack, but what can you do!)
void SwapToViewMatrix()
{
  D3DXMATRIX identity;
  g_pd3dDevice->SetTransform(D3DTS_WORLD, D3DXMatrixIdentity(&identity));
  s_pTargetMatrixStack->m_transformType = D3DTS_VIEW;
}

void SwapToModelMatrix()
{
  D3DXMATRIX identity;
  g_pd3dDevice->SetTransform(D3DTS_VIEW, D3DXMatrixIdentity(&identity));
  s_pTargetMatrixStack->m_transformType = D3DTS_WORLD;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
  D3DXMATRIX mat;
  D3DXMatrixTranslation(&mat, x, y, z);
  s_pTargetMatrixStack->Multiply(mat);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
  D3DXMATRIX mat;
  D3DXMatrixScaling(&mat, x, y, z);
  s_pTargetMatrixStack->Multiply(mat);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  D3DXMATRIX mat;
  D3DXVECTOR3 axis(x, y, z);
  D3DXMatrixRotationAxis(&mat, &axis, D3DXToRadian(angle));
  s_pTargetMatrixStack->Multiply(mat);
}

static D3DCMPFUNC openGLFuncToDirect3D(GLenum func)
{
  switch (func)
  {
    case GL_LEQUAL:
      return D3DCMP_LESSEQUAL;
    case GL_GREATER:
      return D3DCMP_GREATER;
    default: DEBUG_ASSERT(FALSE);
      return D3DCMP_LESSEQUAL;
  }
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
  g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, openGLFuncToDirect3D(func));
  float cref = s_currentAttribs.alphaRef = (ref < 0) ? 0 : ((ref > 1) ? 1 : ref);
  g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, static_cast<DWORD>(cref * 255.0f));
}

void glDepthFunc(GLenum func)
{
  g_pd3dDevice->SetRenderState(D3DRS_ZFUNC, openGLFuncToDirect3D(func));
}

// Display Lists

GLuint glGenLists(GLsizei range)
{
  int result = s_lastDisplayId;
  for (unsigned i = 0; i < range; i++)
    s_displayLists[s_lastDisplayId++] = nullptr;

  return result;
}

void glDeleteLists(GLuint list, GLsizei range)
{
  for (unsigned i = 0; i < range; i++)
  {
    delete s_displayLists[list + i];
    s_displayLists.erase(list + i);
  }
}

void glNewList(GLuint list, GLenum mode)
{
  s_pDisplayListDevice = NEW DisplayListDevice(list);
  s_pModelViewMatrixStack = NEW MatrixStackDisplayList(D3DTS_WORLD, s_pDisplayListDevice, s_pModelViewMatrixStackActual);
  s_pProjectionMatrixStack = NEW MatrixStackDisplayList(D3DTS_PROJECTION, s_pDisplayListDevice, s_pProjectionMatrixStackActual);
  s_pTargetMatrixStack = s_matrixMode == GL_MODELVIEW ? s_pModelViewMatrixStack : s_pProjectionMatrixStack;

  g_pd3dDevice = s_pDisplayListDevice;
}

void glEndList()
{
  DEBUG_ASSERT(s_pDisplayListDevice != NULL);
  if (!s_pDisplayListDevice)
    return;

  s_displayLists[s_pDisplayListDevice->GetId()] = s_pDisplayListDevice->Compile();

  g_pd3dDevice = g_pd3dDeviceActual;

  delete s_pDisplayListDevice;
  delete s_pModelViewMatrixStack;
  delete s_pProjectionMatrixStack;

  s_pDisplayListDevice = nullptr;
  s_pModelViewMatrixStack = s_pModelViewMatrixStackActual;
  s_pProjectionMatrixStack = s_pProjectionMatrixStackActual;
  s_pTargetMatrixStack = s_matrixMode == GL_MODELVIEW ? s_pModelViewMatrixStack : s_pProjectionMatrixStack;
}

void glCallList(GLuint list)
{
  DisplayList* dl = s_displayLists[list];

  if (dl)
  {
    s_pTargetMatrixStack->FastSetTransform();
    dl->Draw();
  }
}

// Materials
void glMaterialfv(GLenum face, GLenum pname, const GLfloat* params)
{
  // This is inefficient, since the OpenGL source code will call glMaterialfv multiple times.
  // to set a series of material properties. Better to do the SetMaterial just before a primitive
  // rendering if necessary.

  D3DMATERIAL9 mat;
  g_pd3dDevice->GetMaterial(&mat);

  switch (pname)
  {
    case GL_SPECULAR:
      mat.Specular.r = params[0];
      mat.Specular.g = params[1];
      mat.Specular.b = params[2];
      mat.Specular.a = params[3];
      break;

    case GL_DIFFUSE:
      mat.Diffuse.r = params[0];
      mat.Diffuse.g = params[1];
      mat.Diffuse.b = params[2];
      mat.Diffuse.a = params[3];
      break;

    case GL_AMBIENT:
      mat.Ambient.r = params[0];
      mat.Ambient.g = params[1];
      mat.Ambient.b = params[2];
      mat.Ambient.a = params[3];
      break;

    case GL_AMBIENT_AND_DIFFUSE:
      mat.Ambient.r = params[0];
      mat.Ambient.g = params[1];
      mat.Ambient.b = params[2];
      mat.Ambient.a = params[3];

      mat.Diffuse.r = params[0];
      mat.Diffuse.g = params[1];
      mat.Diffuse.b = params[2];
      mat.Diffuse.a = params[3];
      break;

    case GL_SHININESS:
      mat.Power = params[0];
      break;

    default:
      // Unknown material property
      DEBUG_ASSERT(FALSE);
      break;
  }

  g_pd3dDevice->SetMaterial(&mat);
}

void glColorMaterial(GLenum face, GLenum mode)
{
  // Warning: This ignores the face setting
  // glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)

  s_currentAttribs.colorMaterialMode = mode;
  if (s_currentAttribs.colorMaterialEnabled)
    setupColorMaterial();
}

GLboolean glIsEnabled(GLenum cap)
{
  DWORD state;

  switch (cap)
  {
    case GL_ALPHA_TEST:
      g_pd3dDevice->GetRenderState(D3DRS_ALPHATESTENABLE, &state);
      return state == TRUE;

    case GL_BLEND:
      g_pd3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &state);
      return state == TRUE;

    case GL_COLOR_MATERIAL:
      return s_currentAttribs.colorMaterialEnabled;

    case GL_CULL_FACE:
      g_pd3dDevice->GetRenderState(D3DRS_CULLMODE, &state);
      return state != D3DCULL_NONE;

    case GL_DEPTH_TEST:
      g_pd3dDevice->GetRenderState(D3DRS_ZENABLE, &state);
      return state == D3DZB_TRUE;

    case GL_FOG:
      g_pd3dDevice->GetRenderState(D3DRS_FOGENABLE, &state);
      return state == TRUE;

    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    {
      BOOL enabled;
      g_pd3dDevice->GetLightEnable(cap - GL_LIGHT0, &enabled);
      return enabled;
    }

    case GL_LIGHTING:
      g_pd3dDevice->GetRenderState(D3DRS_LIGHTING, &state);
      return state == TRUE;

    case GL_LINE_SMOOTH:
      g_pd3dDevice->GetRenderState(D3DRS_ANTIALIASEDLINEENABLE, &state);
      return state == TRUE;

    case GL_NORMALIZE:
      g_pd3dDevice->GetRenderState(D3DRS_NORMALIZENORMALS, &state);
      return state == TRUE;

    case GL_POINT_SMOOTH:
      return FALSE;

    case GL_SCISSOR_TEST:
      g_pd3dDevice->GetRenderState(D3DRS_SCISSORTESTENABLE, &state);
      return state == TRUE;

    case GL_TEXTURE_2D:
      return s_currentAttribs.texturingEnabled[s_activeTexture];

    default: DEBUG_ASSERT(FALSE);
      break;
  }
  return 0;
}

void glPointSize(GLfloat size)
{
  g_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, *(DWORD*)&size);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
  DEBUG_ASSERT(target == GL_TEXTURE_2D);

  switch (pname)
  {
    case GL_TEXTURE_MAG_FILTER:
    {
      DWORD magValue;

      g_pd3dDevice->GetSamplerState(s_activeTexture, D3DSAMP_MAGFILTER, &magValue);
      params[0] = direct3DMinMagFilterToOpenGL(magValue);
    }
    break;

    case GL_TEXTURE_MIN_FILTER:
    {
      DWORD minValue, mipValue;

      g_pd3dDevice->GetSamplerState(s_activeTexture, D3DSAMP_MINFILTER, &minValue);
      g_pd3dDevice->GetSamplerState(s_activeTexture, D3DSAMP_MIPFILTER, &mipValue);

      if (mipValue == D3DTEXF_LINEAR)
        params[0] = GL_LINEAR_MIPMAP_LINEAR;
      else
        params[0] = direct3DMinMagFilterToOpenGL(minValue);
    }
    break;

    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    {
      DWORD wrapState;
      g_pd3dDevice->GetRenderState(D3DRS_WRAP0, &wrapState);

      DWORD wrapMask = (pname == GL_TEXTURE_WRAP_S ? D3DWRAP_U : D3DWRAP_V);

      params[0] = (wrapState & wrapMask ? GL_REPEAT : GL_CLAMP);
      break;
    }

    default: DEBUG_ASSERT(FALSE);
  }
}

// LegacyTexture Environment Mapping

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
  switch (target)
  {
    case GL_TEXTURE_ENV:
      switch (pname)
      {
        case GL_TEXTURE_ENV_MODE:
          s_activeTextureState->envMode = param;
          if (s_currentAttribs.texturingEnabled[s_activeTexture])
            setupTexturing();
          return;

        case GL_COMBINE_RGB_EXT:
          s_activeTextureState->rgbCombineMode = param;
          if (s_currentAttribs.texturingEnabled[s_activeTexture])
            setupTexturing();
          return;
      }
      break;
  }
  DEBUG_ASSERT(false);
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
  switch (target)
  {
    case GL_TEXTURE_ENV:
      switch (pname)
      {
        case GL_TEXTURE_ENV_MODE:
          s_activeTextureState->envMode = param;
          if (s_currentAttribs.texturingEnabled[s_activeTexture])
            setupTexturing();
          return;
      }
      break;
  }
  DEBUG_ASSERT(false);
}

void glTexEnviv(GLenum target, GLenum pname, const GLint* params)
{
  switch (target)
  {
    case GL_TEXTURE_ENV:
      switch (pname)
      {
        case GL_TEXTURE_ENV_COLOR:
        {
          TextureState& s = *s_activeTextureState;

          s.envColor.r = params[0] / 255.0f;
          s.envColor.g = params[1] / 255.0f;
          s.envColor.b = params[2] / 255.0f;
          s.envColor.a = params[3] / 255.0f;
          return;
        }
      }
  }
  DEBUG_ASSERT(false);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint* params)
{
  TextureState& s = *s_activeTextureState;

  switch (target)
  {
    case GL_TEXTURE_ENV:
      switch (pname)
      {
        case GL_TEXTURE_ENV_COLOR:
          params[0] = s.envColor.r * 255.0f;
          params[1] = s.envColor.g * 255.0f;
          params[2] = s.envColor.b * 255.0f;
          params[3] = s.envColor.a * 255.0f;
          return;

        case GL_TEXTURE_ENV_MODE:
          params[0] = s.envMode;
          return;
      }
  }
}

const GLubyte* gluErrorString(GLenum errCode)
{
  return (const GLubyte*)"[Error Reporting Not Yet Implemented]";
}

void glClipPlane(GLenum plane, const GLdouble* equation)
{
  int planeIndex = plane - GL_CLIP_PLANE0;

  float equationAsFloats[4] = {
    static_cast<float>(equation[0]),
    static_cast<float>(equation[1]),
    static_cast<float>(equation[2]),
    static_cast<float>(equation[3])
  };

  g_pd3dDevice->SetClipPlane(planeIndex, equationAsFloats);
}
