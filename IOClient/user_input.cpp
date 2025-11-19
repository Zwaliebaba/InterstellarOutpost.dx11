#include "pch.h"
#include "user_input.h"
#include "GameApp.h"
#include "camera.h"
#include "NetworkClient.h"
#include "Canvas.h"
#include "debugmenu.h"
#include "global_world.h"
#include "input.h"

#include "location.h"
#include "math_utils.h"
#include "preferences.h"
#include "profiler.h"
#include "targetcursor.h"
#include "DX9TextRenderer.h"


UserInput::UserInput()
  : m_removeTopLevelMenu(false)
{
}

// *** AdvanceMenus
void UserInput::AdvanceMenus()
{
  //	if ( g_keyDeltas[KEY_F1] )
  //		DebugKeyBindings::DebugMenu();

  InputManager* im = g_inputManager;
  int mouseX = g_target->X();
  int mouseY = g_target->Y();
  bool lmb = im->controlEvent(ControlEclipseLMousePressed);
  bool rmb = im->controlEvent(ControlEclipseRMousePressed);

  Canvas::EclUpdateMouse(mouseX, mouseY, lmb, rmb);
  Canvas::EclUpdate();

  if (im->controlEvent(ControlEclipseLMouseDown))
  {
    GuiWindow* winUnderMouse = Canvas::EclGetWindow(mouseX, mouseY);
    if (winUnderMouse)
      im->suppressEvent(ControlEclipseLMouseDown);
  }
}

// *** Advance
void UserInput::Advance()
{
  START_PROFILE(g_app->m_profiler, "Advance UserInput");

  g_inputManager->Advance();

  if (m_removeTopLevelMenu)
  {
    GuiWindow* win = Canvas::EclGetWindow(Strings::Get("dialog_toolsmenu"));
    if (win)
      Canvas::EclRemoveWindow(win->m_name);
    m_removeTopLevelMenu = false;
  }

  AdvanceMenus();

  if (g_inputManager->controlEvent(ControlGamePause))
    g_app->m_networkClient->RequestPause();

#ifdef CHEATMENU_ENABLED
  if (g_inputManager->controlEvent(ControlToggleCheatMenu))
    DebugKeyBindings::CheatButton();
#endif

  END_PROFILE(g_app->m_profiler, "Advance UserInput");
}

// *** Render
void UserInput::Render()
{
  START_PROFILE(g_app->m_profiler, "Render UserInput");

  //
  // Render 2D overlays

  g_editorFont.BeginText2D();

  //
  // Eclipse

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(false);

  Canvas::Render();

  glDepthMask(true);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  g_editorFont.EndText2D();

  END_PROFILE(g_app->m_profiler, "Render UserInput");
}

// *** GetMousePos3d
LegacyVector3 UserInput::GetMousePos3d() { return m_mousePos3d; }

// *** RecalcMousePos3d
void UserInput::RecalcMousePos3d()
{
  // Get click ray
  LegacyVector3 rayStart;
  LegacyVector3 rayDir;
  g_app->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);
  ASSERT_VECTOR3_IS_SANE(rayStart);
  ASSERT_VECTOR3_IS_SANE(rayDir);
  rayStart += rayDir * 0.0f;

  bool landscapeHit;
  if (g_app->m_location)
    landscapeHit = g_app->m_location->m_landscape.RayHit(rayStart, rayDir, &m_mousePos3d);
  else
  {
    // We are in the global world
    // So hit against the outer sphere

    LegacyVector3 sphereCenter(0, 0, 0);
    float sphereRadius = 36000.0f;

    rayStart += rayDir * (sphereRadius * 4.0f);
    rayDir = -rayDir;
    landscapeHit = RaySphereIntersection(rayStart, rayDir, sphereCenter, sphereRadius, 1e10, &m_mousePos3d);
    return;
  }

  if (!landscapeHit)
  {
    // OK, we didn't hit against the landscape mesh, so hit against a sphere that
    // encloses the whole world
    LegacyVector3 sphereCenter;
    sphereCenter.x = g_app->m_globalWorld->GetSize() * 0.5f;
    sphereCenter.y = 0.0f;
    sphereCenter.z = g_app->m_globalWorld->GetSize() * 0.5f;

    float sphereRadius = g_app->m_globalWorld->GetSize() * 40.0f;

    rayStart += rayDir * (sphereRadius * 4.0f);
    rayDir = -rayDir;
    landscapeHit = RaySphereIntersection(rayStart, rayDir, sphereCenter, sphereRadius, 1e10, &m_mousePos3d);
  }
}
