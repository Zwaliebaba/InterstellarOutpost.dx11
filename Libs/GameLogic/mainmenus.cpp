#include "pch.h"
#include "mainmenus.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "global_world.h"
#include "prefs_graphics_window.h"
#include "prefs_keybindings_window.h"
#include "prefs_other_window.h"
#include "prefs_screen_window.h"
#include "prefs_sound_window.h"
#include "Renderer.h"
#include "resource.h"
#include "userprofile_window.h"

class WebsiteButton;

class AboutDarwiniaButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("about_darwinia")))
      Canvas::EclRegisterWindow(NEW AboutDarwiniaWindow(), m_parent);
  }
};

class MainMenuUserProfileButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_profile")))
      Canvas::EclRegisterWindow(NEW UserProfileWindow(), m_parent);
  }
};

class OptionsButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_options")))
      Canvas::EclRegisterWindow(NEW OptionsMenuWindow(), m_parent);
  }
};

class ScreenOptionsButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_screenoptions")))
      Canvas::EclRegisterWindow(NEW PrefsScreenWindow(), m_parent);
  }
};

class GraphicsOptionsButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_graphicsoptions")))
      Canvas::EclRegisterWindow(NEW PrefsGraphicsWindow(), m_parent);
  }
};

class SoundOptionsButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_soundoptions")))
      Canvas::EclRegisterWindow(NEW PrefsSoundWindow(), m_parent);
  }
};

class OtherOptionsButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_otheroptions")))
      Canvas::EclRegisterWindow(NEW PrefsOtherWindow(), m_parent);
  }
};

class KeybindingsOptionsButton : public GuiButton
{
  void MouseUp() override
  {
    if (!Canvas::EclGetWindow(Strings::Get("dialog_inputoptions")))
      Canvas::EclRegisterWindow(NEW PrefsKeybindingsWindow, m_parent);
  }
};

MainMenuWindow::MainMenuWindow()
  : GuiWindow(Strings::Get("dialog_mainmenu"))
{
  int screenW = ClientEngine::OutputSize().Width;
  int screenH = ClientEngine::OutputSize().Height;

  SetSize(220, 260);
  SetPosition(screenW / 2.0f - m_w / 2.0f, screenH / 2.0f - m_h / 2.0f);
}

void MainMenuWindow::Render(bool _hasFocus) { GuiWindow::Render(_hasFocus); }

OptionsMenuWindow::OptionsMenuWindow()
  : GuiWindow(Strings::Get("dialog_options"))
{
  SetSize(240, 230);
}

void OptionsMenuWindow::Create()
{
  GuiWindow::Create();

  int fontSize = GetMenuSize(13);
  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - border * 2;
  int h = buttonH + border;

  auto screen = NEW ScreenOptionsButton();
  screen->SetProperties(Strings::Get("dialog_screenoptions"), border, y += border, buttonW, buttonH);
  screen->m_fontSize = fontSize;
  screen->m_centered = true;
  RegisterButton(screen);
  m_buttonOrder.PutData(screen);

  auto graphics = NEW GraphicsOptionsButton();
  graphics->SetProperties(Strings::Get("dialog_graphicsoptions"), border, y += h, buttonW, buttonH);
  graphics->m_fontSize = fontSize;
  graphics->m_centered = true;
  RegisterButton(graphics);
  m_buttonOrder.PutData(graphics);

  auto sound = NEW SoundOptionsButton();
  sound->SetProperties(Strings::Get("dialog_soundoptions"), border, y += h, buttonW, buttonH);
  sound->m_fontSize = fontSize;
  sound->m_centered = true;
  RegisterButton(sound);
  m_buttonOrder.PutData(sound);

  auto keys = NEW KeybindingsOptionsButton();
  keys->SetProperties(Strings::Get("dialog_inputoptions"), border, y += h, buttonW, buttonH);
  keys->m_fontSize = fontSize;
  keys->m_centered = true;
  RegisterButton(keys);
  m_buttonOrder.PutData(keys);

  auto other = NEW OtherOptionsButton();
  other->SetProperties(Strings::Get("dialog_otheroptions"), border, y += h, buttonW, buttonH);
  other->m_fontSize = fontSize;
  other->m_centered = true;
  RegisterButton(other);
  m_buttonOrder.PutData(other);

  auto close = NEW CloseButton();
  close->SetProperties(Strings::Get("dialog_close"), border, m_h - h, buttonW, buttonH);
  close->m_fontSize = fontSize;
  close->m_centered = true;
  RegisterButton(close);
  m_buttonOrder.PutData(close);
}

class ExitLevelButton : public GuiButton
{
  void MouseUp() override
  {
    Canvas::EclRemoveWindow(m_parent->m_name);

    g_app->m_requestedLocationId = -1;
  }
};

LocationWindow::LocationWindow()
  : GuiWindow(Strings::Get("dialog_locationmenu"))
{
  int screenW = ClientEngine::OutputSize().Width;
  int screenH = ClientEngine::OutputSize().Height;

  SetSize(200, 220);
  SetPosition(screenW / 2.0f - m_w / 2.0f, screenH / 2.0f - m_h / 2.0f);
}

void LocationWindow::Create()
{
  GuiWindow::Create();

  int fontSize = GetMenuSize(13);
  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - border * 2;
  int h = buttonH + border;

  int gap = border;

  GlobalLocation* loc = g_app->m_globalWorld->GetLocation(g_app->m_locationId);

  auto exitLevel = NEW ExitLevelButton();
  exitLevel->SetProperties(Strings::Get("dialog_leavelocation"), border, y += gap, buttonW, buttonH);
  exitLevel->m_fontSize = fontSize;
  exitLevel->m_centered = true;
  RegisterButton(exitLevel);
  m_buttonOrder.PutData(exitLevel);

  auto options = NEW OptionsButton();
  options->SetProperties(Strings::Get("dialog_options"), border, y += h, buttonW, buttonH);
  options->m_fontSize = fontSize;
  options->m_centered = true;
  RegisterButton(options);
  m_buttonOrder.PutData(options);

  auto close = NEW CloseButton();
  close->SetProperties(Strings::Get("dialog_close"), border, m_h - h, buttonW, buttonH);
  close->m_fontSize = fontSize;
  close->m_centered = true;
  RegisterButton(close);
  m_buttonOrder.PutData(close);
}

void MainMenuWindow::Create()
{
  GuiWindow::Create();

  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - border * 2;
  int h = buttonH + border;

  int fontSize = GetMenuSize(13);

  auto profile = NEW MainMenuUserProfileButton();
  profile->SetProperties(Strings::Get("dialog_profile"), border, y += border, buttonW, buttonH);
  profile->m_fontSize = fontSize;
  profile->m_centered = true;
  RegisterButton(profile);
  m_buttonOrder.PutData(profile);

  auto options = NEW OptionsButton();
  options->SetProperties(Strings::Get("dialog_options"), border, y += h, buttonW, buttonH);
  options->m_fontSize = fontSize;
  options->m_centered = true;
  RegisterButton(options);
  m_buttonOrder.PutData(options);

  auto exit = NEW GameExitButton();
  exit->SetProperties(Strings::Get("dialog_leavedarwinia"), border, y += h, buttonW, buttonH);
  exit->m_fontSize = fontSize;
  exit->m_centered = true;
  RegisterButton(exit);
  m_buttonOrder.PutData(exit);

  auto close = NEW CloseButton();
  close->SetProperties(Strings::Get("dialog_close"), border, m_h - h, buttonW, buttonH);
  close->m_fontSize = fontSize;
  close->m_centered = true;
  RegisterButton(close);
  m_buttonOrder.PutData(close);
}

AboutDarwiniaWindow::AboutDarwiniaWindow()
  : GuiWindow(Strings::Get("about_darwinia")) { SetSize(350, 250); }

void AboutDarwiniaWindow::Create()
{
  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - border * 2;
  int h = buttonH + border;
  int fontSize = GetMenuSize(13);

  auto close = NEW CloseButton();
  close->SetProperties(Strings::Get("dialog_close"), border, m_h - h, buttonW, buttonH);
  close->m_fontSize = fontSize;
  close->m_centered = true;
  RegisterButton(close);
  m_buttonOrder.PutData(close);
}

void AboutDarwiniaWindow::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites\\darwinian.bmp"));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  float texH = 1.0f;
  float texW = texH * 512.0f / 64.0f;

  glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(m_x + m_w / 2 - 25, m_y + 30);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(m_x + m_w / 2 + 25, m_y + 30);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(m_x + m_w / 2 + 25, m_y + GetMenuSize(80));
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(m_x + m_w / 2 - 25, m_y + GetMenuSize(80));
  glEnd();

  glDisable(GL_TEXTURE_2D);

  float y = m_y + 100;
  float h = GetMenuSize(18);

  float fontSize = GetMenuSize(13);

  char about[512];
  sprintf(about, "%s %s", Strings::Get("bootloader_credits_4").c_str(), Strings::Get("bootloader_credits_5").c_str());

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  g_gameFont.DrawText2DCenter(m_x + m_w / 2, y += h, fontSize, "Deep Space Outpost");
  g_gameFont.DrawText2DCenter(m_x + m_w / 2, y += 2 * h, fontSize, about);
}
