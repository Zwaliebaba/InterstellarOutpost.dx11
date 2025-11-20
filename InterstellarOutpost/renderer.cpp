#include "pch.h"
#include "Renderer.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "StartSequence.h"
#include "camera.h"
#include "explosion.h"
#include "gamecursor.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "input.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "ogl_extensions.h"
#include "ParticleSystem.h"
#include "profiler.h"
#include "resource.h"
#include "taskmanager_interface.h"
#include "unit.h"
#include "user_input.h"
#include "window_manager.h"

#define USE_PIXEL_EFFECT_GRID_OPTIMISATION	1

Renderer::Renderer()
  : m_fps(60),
    m_displayFPS(false),
    m_renderDebug(false),
    m_displayInputMode(false),
    m_nearPlane(5.0f),
    m_farPlane(150000.0f),
    m_screenW(0),
    m_screenH(0),
    m_tileIndex(0),
    m_fadedness(0.0f),
    m_fadeRate(0.0f),
    m_fadeDelay(0.0f)
{}

void Renderer::Initialize()
{
  m_screenW = (int)ClientEngine::OutputSize().Width;
  m_screenH = (int)ClientEngine::OutputSize().Height;

  InitialiseOGLExtensions();
}

void Renderer::Render()
{
#ifdef PROFILER_ENABLED
  g_app->m_profiler->RenderStarted();
#endif

  RenderFrame();

#ifdef PROFILER_ENABLED
  g_app->m_profiler->RenderEnded();
#endif // PROFILER_ENABLED
}

bool Renderer::IsFadeComplete() const
{
  if (NearlyEquals(m_fadeRate, 0.0f))
    return true;

  return false;
}

void Renderer::StartFadeOut()
{
  m_fadeDelay = 0.0f;
  m_fadeRate = 1.0f;
}

void Renderer::StartFadeIn(float _delay)
{
  m_fadedness = 1.0f;
  m_fadeDelay = _delay;
  m_fadeRate = -1.0f;
}

void Renderer::RenderFadeOut()
{
  static float lastTime = GetHighResTime();
  float timeNow = GetHighResTime();
  float timeIncrement = timeNow - lastTime;
  if (timeIncrement > 0.05f)
    timeIncrement = 0.05f;
  lastTime = timeNow;

  if (m_fadeDelay > 0.0f)
    m_fadeDelay -= timeIncrement;
  else
  {
    m_fadedness += m_fadeRate * timeIncrement;
    if (m_fadedness < 0.0f)
    {
      m_fadedness = 0.0f;
      m_fadeRate = 0.0f;
    }
    else if (m_fadedness > 1.0f)
    {
      m_fadeRate = 0.0f;
      m_fadedness = 1.0f;
    }
  }

  if (m_fadedness > 0.0001f)
  {
    glEnable(GL_BLEND);
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4ub(0, 0, 0, static_cast<int>(m_fadedness * 255.0f));
    glBegin(GL_QUADS);
    glVertex2i(-1, -1);
    glVertex2i(m_screenW, -1);
    glVertex2i(m_screenW, m_screenH);
    glVertex2i(-1, m_screenH);
    glEnd();

    glDisable(GL_BLEND);
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
}

void Renderer::RenderFrame(bool withFlip)
{
  SetOpenGLState();

  if (g_app->m_locationId == -1)
  {
    m_nearPlane = 50.0f;
    m_farPlane = 10000000.0f;
  }
  else
  {
    m_nearPlane = 5.0f;
    m_farPlane = 15000.0f;
  }

  FPSMeterAdvance();
  SetupMatricesFor3D();

  RGBAColor* col = &g_app->m_backgroundColor;
  if (g_app->m_location)
    glClearColor(col->r / 255.0f, col->g / 255.0f, col->b / 255.0f, col->a / 255.0f);
  else
    glClearColor(0.05f, 0.0f, 0.05f, 0.1f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  if (g_app->m_locationId != -1)
  {
      g_app->m_location->Render();
  }
  else
    g_app->m_globalWorld->Render();

  g_explosionManager.Render();
  g_app->m_particleSystem->Render();

  g_app->m_userInput->Render();
  g_app->m_gameCursor->Render();
  g_app->m_taskManagerInterface->Render();

  g_editorFont.BeginText2D();

  RenderFadeOut();

  if (m_displayFPS)
  {
    glColor4f(0, 0, 0, 0.6);
    glBegin(GL_QUADS);
    glVertex2f(8.0, 1.0f);
    glVertex2f(70.0, 1.0f);
    glVertex2f(70.0, 15.0f);
    glVertex2f(8.0, 15.0f);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    g_editorFont.DrawText2D(12, 10, DEF_FONT_SIZE, "FPS: %d", m_fps);
  }

  if (m_displayInputMode)
  {
    glColor4f(0, 0, 0, 0.6);
    glBegin(GL_QUADS);
    glVertex2f(80.0, 1.0f);
    glVertex2f(230.0, 1.0f);
    glVertex2f(230.0, 18.0f);
    glVertex2f(80.0, 18.0f);
    glEnd();

    std::string inmode;
    switch (g_inputManager->getInputMode())
    {
      case INPUT_MODE_KEYBOARD:
        inmode = "keyboard";
        break;
      case INPUT_MODE_GAMEPAD:
        inmode = "gamepad";
        break;
      default:
        inmode = "unknown";
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    g_editorFont.DrawText2D(84, 10, DEF_FONT_SIZE, "InputMode: %s", inmode.c_str());
  }

  int latency = Server::m_sequenceId - g_lastProcessedSequenceId;
  if (latency > 10)
  {
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(m_screenW / 2 - 200, 120);
    glVertex2f(m_screenW / 2 + 200, 120);
    glVertex2f(m_screenW / 2 + 200, 80);
    glVertex2f(m_screenW / 2 - 200, 80);
    glEnd();
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    g_editorFont.DrawText2DCenter(m_screenW / 2, 100, 20, "Client LAG %dms behind Server ", latency * 100);
  }

  if (g_app->m_startSequence)
    g_app->m_startSequence->Render();

  g_editorFont.EndText2D();

  if (withFlip)
    g_windowManager->Flip();
}

void Renderer::SetupProjMatrixFor3D() const
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(g_app->m_camera->GetFov(), static_cast<float>(m_screenW) / static_cast<float>(m_screenH), // Aspect ratio
                 m_nearPlane, m_farPlane);
}

void Renderer::SetupMatricesFor3D() const
{
  Camera* camera = g_app->m_camera;

  SetupProjMatrixFor3D();
  camera->SetupModelviewMatrix();
}

void Renderer::SetupMatricesFor2D() const
{
  int v[4];
  glGetIntegerv(GL_VIEWPORT, &v[0]);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  float left = v[0];
  float right = v[0] + v[2];
  float bottom = v[1] + v[3];
  float top = v[1];

  gluOrtho2D(left, right, bottom, top);
  glMatrixMode(GL_MODELVIEW);
}

void Renderer::FPSMeterAdvance()
{
  static int framesThisSecond = 0;
  static float endOfSecond = GetHighResTime() + 1.0;

  framesThisSecond++;

  float currentTime = GetHighResTime();
  if (currentTime > endOfSecond)
  {
    if (currentTime > endOfSecond + 2.0)
      endOfSecond = currentTime + 1.0;
    else
      endOfSecond += 1.0;
    m_fps = framesThisSecond;
    framesThisSecond = 0;
  }
}

float Renderer::GetNearPlane() const { return m_nearPlane; }

float Renderer::GetFarPlane() const { return m_farPlane; }

void Renderer::SetNearAndFar(float _nearPlane, float _farPlane)
{
  DEBUG_ASSERT(_nearPlane < _farPlane);
  DEBUG_ASSERT(_nearPlane > 0.0f);
  m_nearPlane = _nearPlane;
  m_farPlane = _farPlane;
}

void Renderer::SetOpenGLState() const
{
  // Geometry
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_FILL);
  glShadeModel(GL_FLAT);
  glDisable(GL_NORMALIZE);

  // Color
  glDisable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  // Lighting
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHT2);
  glDisable(GL_LIGHT3);
  glDisable(GL_LIGHT4);
  glDisable(GL_LIGHT5);
  glDisable(GL_LIGHT6);
  glDisable(GL_LIGHT7);
  if (g_app->m_location)
    g_app->m_location->SetupLights();
  else
    g_app->m_globalWorld->SetupLights();
  float ambient[] = {0, 0, 0, 0};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

  // Blending, Anti-aliasing, Fog and Polygon Offset
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.01);
  if (g_app->m_location)
    g_app->m_location->SetupFog();
  else
    g_app->m_globalWorld->SetupFog();
  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_POINT_SMOOTH);

  // LegacyTexture Mapping
  glDisable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  int colour[4] = {0, 0, 0, 0};
  glTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour);

  // Frame Buffer
  glEnable(GL_DEPTH_TEST);
  glDepthMask(true);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_SCISSOR_TEST);
}

void Renderer::SetObjectLighting() const
{
  float spec = 0.0f;
  float diffuse = 1.0f;
  float amb = 0.0f;
  GLfloat materialShininess[] = {127.0f};
  GLfloat materialSpecular[] = {spec, spec, spec, 0.0f};
  GLfloat materialDiffuse[] = {diffuse, diffuse, diffuse, 1.0f};
  GLfloat ambCol[] = {amb, amb, amb, 1.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
  glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
}

void Renderer::UnsetObjectLighting() const
{
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
}
