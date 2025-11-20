#include "pch.h"
#include "debugmenu.h"
#include "GameApp.h"
#include "cheat_window.h"
#include "network_window.h"
#include "prefs_graphics_window.h"
#include "prefs_screen_window.h"
#include "prefs_sound_window.h"
#include "profilewindow.h"
#include "reallyquit_window.h"
#include "Renderer.h"

#ifdef PROFILER_ENABLED
class ProfileButton : public GuiButton
{
  public:
    void MouseUp() override { DebugKeyBindings::ProfileButton(); }
};
#endif // PROFILER_ENABLED

class NetworkButton : public GuiButton
{
  public:
    void MouseUp() override { DebugKeyBindings::NetworkButton(); }
};

class FPSButton : public GuiButton
{
  public:
    void MouseUp() override { DebugKeyBindings::FPSButton(); }
};

class PrefsScreenButton : public GuiButton
{
  public:
    void MouseUp() override
    {
      if (!Canvas::EclGetWindow(Strings::Get("dialog_screenoptions")))
        Canvas::EclRegisterWindow(NEW PrefsScreenWindow());
    }
};

class PrefsGfxDetailButton : public GuiButton
{
  public:
    void MouseUp() override
    {
      if (!Canvas::EclGetWindow(Strings::Get("dialog_graphicsoptions")))
        Canvas::EclRegisterWindow(NEW PrefsGraphicsWindow());
    }
};

class PrefsSoundButton : public GuiButton
{
  public:
    void MouseUp() override
    {
      if (!Canvas::EclGetWindow(Strings::Get("dialog_soundoptions")))
        Canvas::EclRegisterWindow(NEW PrefsSoundWindow());
    }
};

#ifdef CHEATMENU_ENABLED
class CheatButton : public GuiButton
{
  public:
    void MouseUp() override { DebugKeyBindings::CheatButton(); }
};
#endif

DebugMenu::DebugMenu(std::string_view name)
  : GuiWindow(name)
{
  m_x = 10;
  m_y = 20;
  m_w = 170;
  m_h = 75;
}

void DebugMenu::Create()
{
  GuiWindow::Create();

  int pitch = 18;
  int y = 5;

  GuiButton* button;

#ifdef PROFILER_ENABLED
  button = NEW ProfileButton();
  button->SetProperties("Profile (F6)", 10, y += pitch, m_w - 20);
  RegisterButton(button);
#endif // PROFILER_ENABLED

  button = NEW NetworkButton();
  button->SetProperties("Network Stats", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  button = NEW FPSButton();
  button->SetProperties("Display FPS (F5)", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  y += pitch / 2.0f;

#ifdef CHEATMENU_ENABLED
  button = NEW CheatButton();
  button->SetProperties("Cheat Menu (F4)", 10, y += pitch, m_w - 20);
  RegisterButton(button);
#endif
}

void DebugMenu::Render(bool hasFocus)
{
  GuiWindow::Render(hasFocus);
}

void DebugKeyBindings::DebugMenu()
{
  std::string debugMenuWindowName = Strings::Get("dialog_toolsmenu");
  if (Canvas::EclGetWindow(debugMenuWindowName))
    Canvas::EclRemoveWindow(debugMenuWindowName);
  else
    Canvas::EclRegisterWindow(NEW ::DebugMenu(debugMenuWindowName));
}

#ifdef PROFILER_ENABLED
void DebugKeyBindings::ProfileButton()
{
  if (Canvas::EclGetWindow("Profiler"))
    Canvas::EclRemoveWindow("Profiler");
  else
  {
    auto pw = NEW ProfileWindow("Profiler");
    pw->m_w = 570;
    pw->m_h = 450;
    pw->m_x = ClientEngine::OutputSize().Width - pw->m_w - 20;
    pw->m_y = 30;
    Canvas::EclRegisterWindow(pw);
  }
}
#endif

void DebugKeyBindings::NetworkButton()
{
  if (!Canvas::EclGetWindow("Network Stats"))
  {
    auto nw = NEW NetworkWindow("Network Stats");
    nw->m_w = 200;
    nw->m_h = 200;
    nw->m_x = 10;
    nw->m_y = ClientEngine::OutputSize().Height - nw->m_h;
    Canvas::EclRegisterWindow(nw);
  }
}

void DebugKeyBindings::FPSButton() { g_app->m_renderer->m_displayFPS = !g_app->m_renderer->m_displayFPS; }

#ifdef CHEATMENU_ENABLED
void DebugKeyBindings::CheatButton()
{
  if (!Canvas::EclGetWindow("Cheat Window"))
  {
    auto window = NEW CheatWindow("Cheat Window");
    window->m_w = 200;
    window->m_h = 200;
    window->m_x = 250;
    window->m_y = 50;
    Canvas::EclRegisterWindow(window);
  }
}
#endif

void DebugKeyBindings::ReallyQuitButton()
{
  // Bring up a really quit window
  if (!Canvas::EclGetWindow(REALLYQUIT_WINDOWNAME))
    Canvas::EclRegisterWindow(NEW ReallyQuitWindow());
}

void DebugKeyBindings::ToggleFullscreenButton()
{
}
