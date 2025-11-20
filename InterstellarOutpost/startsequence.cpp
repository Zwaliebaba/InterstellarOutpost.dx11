#include "pch.h"
#include "StartSequence.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "camera.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "input.h"
#include "profiler.h"
#include "Renderer.h"
#include "soundsystem.h"
#include "user_input.h"

StartSequence::StartSequence()
{
  m_startTime = GetHighResTime();

  float screenRatio = ClientEngine::OutputSize().Height / static_cast<float>(ClientEngine::OutputSize().Width);
  int screenH = 800 * screenRatio;

  float x = 150;
  float y = screenH * 4 / 5.0f;
  float size = 10.0f;

  RegisterCaption(Strings::Get("intro_1"), x, y, size, 3, 15);
  RegisterCaption(Strings::Get("intro_2"), x, y + 15, 20, 8, 15);
  RegisterCaption(Strings::Get("intro_3"), x, y + 40, size, 30, 45);
  RegisterCaption(Strings::Get("intro_4"), x, y + 50, size, 42, 45);
  RegisterCaption(Strings::Get("intro_5"), x, y, size, 54, 65);
  RegisterCaption(Strings::Get("intro_6"), x, y, size, 66, 74);
  RegisterCaption(Strings::Get("intro_7"), x, y + 15, size, 72, 74);
  RegisterCaption(Strings::Get("intro_8"), x, y, size, 74, 90);
  RegisterCaption(Strings::Get("intro_9"), x, y + 15, 15, 82, 90);
  RegisterCaption(Strings::Get("intro_10"), x, y + 30, 15, 86, 90);
}

void StartSequence::RegisterCaption(std::string_view _caption, float _x, float _y, float _size, float _startTime, float _endTime)
{
  StartSequenceCaption caption;
  caption.m_caption = _caption;
  caption.m_x = _x;
  caption.m_y = _y;
  caption.m_size = _size;
  caption.m_startTime = _startTime;
  caption.m_endTime = _endTime;
  m_captions.emplace_back(caption);
}

bool StartSequence::Advance()
{
  static bool started = false;
  if (GetHighResTime() > m_startTime && !started)
  {
    started = true;
    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "StartSequence", SoundSourceBlueprint::TypeMusic);
    g_app->m_camera->RequestMode(Camera::ModeSphereWorldIntro);
  }

  g_inputManager->PollForEvents();

  if (g_inputManager->controlEvent(ControlMenuEscape) || g_app->m_requestQuit || (GetHighResTime() - m_startTime) > 90)
  {
    g_app->m_soundSystem->StopAllSounds(WorldObjectId(), "Music StartSequence");
    return true;
  }

  g_app->m_userInput->Advance();
  g_app->m_camera->Advance();
  g_app->m_soundSystem->Advance();
  g_app->m_profiler->Advance();

  g_app->m_renderer->Render();

  return false;
}

void StartSequence::Render()
{
  float screenRatio = static_cast<float>(ClientEngine::OutputSize().Height) / static_cast<float>(ClientEngine::OutputSize().Width);
  int screenH = 800 * screenRatio;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 800, screenH, 0);
  glMatrixMode(GL_MODELVIEW);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  float timeNow = GetHighResTime() - m_startTime;

  if (timeNow < 3.0f)
  {
    float alpha = 1.0f - timeNow / 3.0f;
    glColor4f(0, 0, 0, alpha);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(800, 0);
    glVertex2i(800, screenH);
    glVertex2i(0, screenH);
    glEnd();
  }

  if (timeNow > 87)
  {
    float alpha = (timeNow - 87) / 2.0f;
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(800, 0);
    glVertex2i(800, screenH);
    glVertex2i(0, screenH);
    glEnd();
  }

  LegacyVector2 cursorPos;
  bool cursorFlash = false;
  float cursorSize = 0.0f;

  for (auto& caption : m_captions)
  {
    if (timeNow >= caption.m_startTime && timeNow <= caption.m_endTime)
    {
      std::string theString = caption.m_caption;
      size_t stringLength = theString.size();
      size_t maxTimeLength = (timeNow - caption.m_startTime) * 20;
      theString = theString.substr(0, maxTimeLength);

      glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
      g_gameFont.DrawText2D(caption.m_x, caption.m_y, caption.m_size, theString);

      size_t finishedLen = theString.size();
      int texW = g_gameFont.GetTextWidth(finishedLen, caption.m_size);
      cursorPos.Set(caption.m_x + texW, caption.m_y - 7.25f);
      cursorFlash = maxTimeLength > stringLength;
      cursorSize = caption.m_size;
    }
  }

  if (cursorPos != LegacyVector2(0, 0))
  {
    if (!cursorFlash || fmod(timeNow, 1) < 0.5f)
    {
      glBegin(GL_QUADS);
      glVertex2f(cursorPos.x, cursorPos.y);
      glVertex2f(cursorPos.x + cursorSize * 0.7f, cursorPos.y);
      glVertex2f(cursorPos.x + cursorSize * 0.7f, cursorPos.y + cursorSize * 0.88f);
      glVertex2f(cursorPos.x, cursorPos.y + cursorSize * 0.88f);
      glEnd();
    }
  }

  g_app->m_renderer->SetupMatricesFor3D();

  //
  // Render grid behind darwinia

  float scale = 1000.0f;

  float fog = 0.0f;
  float fogCol[] = {fog, fog, fog, fog};

  int fogVal = 5810000;

  float r = 2.0f;
  float height = -400.0f;
  float gridSize = 100.0f;

  float xStart = -4000.0f * r;
  float xEnd = 4000.0f + 4000.0f * r;
  float zStart = -4000.0f * r;
  float zEnd = 4000.0f + 4000.0f * r;

  float fogColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  if (timeNow > 50.0f)
  {
    glPushMatrix();
    glScalef(scale, scale, scale);

    glFogf(GL_FOG_DENSITY, 1.0f);
    glFogf(GL_FOG_START, 0.0f);
    glFogf(GL_FOG_END, static_cast<float>(fogVal));
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glEnable(GL_FOG);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glColor4f(0.5, 0.5, 1.0, 0.5);

    float percentDrawn = 1.0f - (timeNow - 50.0f) / 10.0f;
    percentDrawn = std::max(percentDrawn, 0.0f);
    xEnd -= (8000 + 4000 * r * percentDrawn);
    zEnd -= (8000 + 4000 * r * percentDrawn);

    for (int x = xStart; x < xEnd; x += gridSize)
    {
      glBegin(GL_LINES);
      glVertex3f(x, height, zStart);
      glVertex3f(x, height, zEnd);
      glVertex3f(xStart, height, x);
      glVertex3f(xEnd, height, x);
      glEnd();
    }

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(true);

    g_app->m_globalWorld->SetupFog();
    glDisable(GL_FOG);

    glPopMatrix();
  }
}
