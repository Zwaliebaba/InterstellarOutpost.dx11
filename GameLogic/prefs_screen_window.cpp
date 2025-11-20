#include "pch.h"

#include "preference_names.h"
#include "preferences.h"
#include "resource.h"
#include "DX9TextRenderer.h"
#include "window_manager.h"
#include "prefs_screen_window.h"
#include "DropDownMenu.h"
#include "GameApp.h"
#include "Renderer.h"

#define HAVE_REFRESH_RATES

class ScreenResDropDownMenu : public DropDownMenu
{
#ifdef HAVE_REFRESH_RATES
  void SelectOption(int _option) override
  {
    auto parent = static_cast<PrefsScreenWindow*>(m_parent);

    DropDownMenu::SelectOption(_option);

    // Update available refresh rates

    auto refresh = static_cast<DropDownMenu*>(m_parent->GetButton(Strings::Get("dialog_refreshrate")));
    refresh->Empty();

    Resolution* resolution = g_windowManager->GetResolution(_option);
    if (resolution)
    {
      for (int i = 0; i < resolution->m_refreshRates.Size(); ++i)
      {
        int thisRate = resolution->m_refreshRates[i];
        char caption[64];
        sprintf(caption, "%d Hz", thisRate);
        refresh->AddOption(caption, thisRate);
      }
      refresh->SelectOption(parent->m_refreshRate);
    }
  }
#endif
};

class FullscreenRequiredMenu : public DropDownMenu
{
  void Render(int realX, int realY, bool highlighted, bool clicked) override
  {
    auto windowed = static_cast<DropDownMenu*>(m_parent->GetButton(Strings::Get("dialog_windowed")));
    bool available = (windowed->GetSelectionValue() == 0);
    if (available)
      DropDownMenu::Render(realX, realY, highlighted, clicked);
    else
    {
      glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
      auto parent = static_cast<GuiWindow*>(m_parent);
      g_editorFont.DrawText2D(realX + 10, realY + 9, parent->GetMenuSize(13), Strings::Get("dialog_windowedmode"));
    }
  }

  void MouseUp() override
  {
    auto windowed = static_cast<DropDownMenu*>(m_parent->GetButton(Strings::Get("dialog_windowed")));
    bool available = (windowed->GetSelectionValue() == 0);
    if (available)
      DropDownMenu::MouseUp();
  }
};

class SetScreenButton : public GuiButton
{
  void MouseUp() override
  {
    auto parent = dynamic_cast<PrefsScreenWindow*>(m_parent);

    Resolution* resolution = g_windowManager->GetResolution(parent->m_resId);
    g_prefsManager->SetInt(SCREEN_WIDTH_PREFS_NAME, resolution->m_width);
    g_prefsManager->SetInt(SCREEN_HEIGHT_PREFS_NAME, resolution->m_height);
#ifdef HAVE_REFRESH_RATES
    g_prefsManager->SetInt(SCREEN_REFRESH_PREFS_NAME, parent->m_refreshRate);
#endif
    g_prefsManager->SetInt(SCREEN_WINDOWED_PREFS_NAME, parent->m_windowed);
    g_prefsManager->SetInt(SCREEN_COLOUR_DEPTH_PREFS_NAME, parent->m_colourDepth);
    g_prefsManager->SetInt(SCREEN_Z_DEPTH_PREFS_NAME, parent->m_zDepth);

    parent->m_resId = g_windowManager->GetResolutionId(g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                                                       g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME));

    parent->m_windowed = g_prefsManager->GetInt(SCREEN_WINDOWED_PREFS_NAME, 0);
    parent->m_colourDepth = g_prefsManager->GetInt(SCREEN_COLOUR_DEPTH_PREFS_NAME, 16);
#ifdef HAVE_REFRESH_RATES
    parent->m_refreshRate = g_prefsManager->GetInt(SCREEN_REFRESH_PREFS_NAME, 60);
#endif
    parent->m_zDepth = g_prefsManager->GetInt(SCREEN_Z_DEPTH_PREFS_NAME, 24);

    parent->Remove();
    parent->Create();
  }
};

PrefsScreenWindow::PrefsScreenWindow()
  : GuiWindow(Strings::Get("dialog_screenoptions"))
{
  int height = 240;

  m_resId = g_windowManager->GetResolutionId(g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                                             g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME));

  m_windowed = g_prefsManager->GetInt(SCREEN_WINDOWED_PREFS_NAME, 0);
  m_colourDepth = g_prefsManager->GetInt(SCREEN_COLOUR_DEPTH_PREFS_NAME, 16);
#ifdef HAVE_REFRESH_RATES
  m_refreshRate = g_prefsManager->GetInt(SCREEN_REFRESH_PREFS_NAME, 60);
  height += 30;
#endif
  m_zDepth = g_prefsManager->GetInt(SCREEN_Z_DEPTH_PREFS_NAME, 24);

  SetSize(410, height);
  SetPosition(ClientEngine::OutputSize().Width / 2 - m_w / 2, ClientEngine::OutputSize().Height / 2 - m_h / 2);
}

void PrefsScreenWindow::Create()
{
  GuiWindow::Create();

  /*int x = GetMenuSize(150);
  int w = GetMenuSize(170);
  int y = 30;
  int h = GetMenuSize(30);
int fontSize = GetMenuSize(13);*/

  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int x = m_w * 0.5;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - x - border * 2;
  int buttonW2 = m_w / 2 - border * 2;
  int h = buttonH + border;
  int fontSize = GetMenuSize(13);

  auto box = NEW InvertedBox();
  box->SetProperties("invert", 10, y += border, m_w - 20, GetClientRectY2() - h * 2);
  RegisterButton(box);

  auto screenRes = NEW ScreenResDropDownMenu();
  screenRes->SetProperties(Strings::Get("dialog_resolution"), x, y += border, buttonW, buttonH);
  for (int i = 0; i < g_windowManager->m_resolutions.Size(); ++i)
  {
    Resolution* resolution = g_windowManager->m_resolutions[i];
    char caption[64];
    sprintf(caption, "%d x %d", resolution->m_width, resolution->m_height);
    screenRes->AddOption(caption, i);
  }
  screenRes->m_fontSize = fontSize;

  auto windowed = NEW DropDownMenu();
  windowed->SetProperties(Strings::Get("dialog_windowed"), x, y += h, buttonW, buttonH);
  windowed->AddOption(Strings::Get("dialog_yes"), 1);
  windowed->AddOption(Strings::Get("dialog_no"), 0);
  windowed->m_fontSize = fontSize;
  windowed->RegisterInt(&m_windowed);

#ifdef HAVE_REFRESH_RATES
  auto refresh = NEW FullscreenRequiredMenu();
  refresh->SetProperties(Strings::Get("dialog_refreshrate"), x, y += h, buttonW, buttonH);
  refresh->RegisterInt(&m_refreshRate);
  refresh->m_fontSize = fontSize;
  RegisterButton(refresh);
#endif

  RegisterButton(windowed);
  RegisterButton(screenRes);
  screenRes->RegisterInt(&m_resId);

  auto colourDepth = NEW FullscreenRequiredMenu();
  colourDepth->SetProperties(Strings::Get("dialog_colourdepth"), x, y += h, buttonW, buttonH);
  colourDepth->AddOption(Strings::Get("dialog_colourdepth_16"), 16);
  colourDepth->AddOption(Strings::Get("dialog_colourdepth_24"), 24);
  colourDepth->AddOption(Strings::Get("dialog_colourdepth_32"), 32);
  colourDepth->RegisterInt(&m_colourDepth);
  colourDepth->m_fontSize = fontSize;
  RegisterButton(colourDepth);

  m_buttonOrder.PutData(screenRes);
  m_buttonOrder.PutData(windowed);
  if (windowed->GetSelectionValue() == 0)
  {
    m_buttonOrder.PutData(refresh);
    m_buttonOrder.PutData(colourDepth);
  }

  auto zDepth = NEW DropDownMenu();
  zDepth->SetProperties(Strings::Get("dialog_zbufferdepth"), x, y += h, buttonW, buttonH);
  zDepth->AddOption(Strings::Get("dialog_colourdepth_16"), 16);
  zDepth->AddOption(Strings::Get("dialog_colourdepth_24"), 24);
  zDepth->RegisterInt(&m_zDepth);
  zDepth->m_fontSize = fontSize;
  RegisterButton(zDepth);
  m_buttonOrder.PutData(zDepth);

  y = m_h - h;

  auto cancel = NEW CloseButton();
  cancel->SetProperties(Strings::Get("dialog_close"), border, y, buttonW2, buttonH);
  cancel->m_fontSize = fontSize;
  cancel->m_centered = true;
  RegisterButton(cancel);
  m_buttonOrder.PutData(cancel);

  auto apply = NEW SetScreenButton();
  apply->SetProperties(Strings::Get("dialog_apply"), m_w - buttonW2 - border, y, buttonW2, buttonH);
  apply->m_fontSize = fontSize;
  apply->m_centered = true;
  RegisterButton(apply);
  m_buttonOrder.PutData(apply);
}

void PrefsScreenWindow::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  int border = GetClientRectX1() + 10;
  int x = m_x + 20;
  int y = m_y + GetClientRectY1() + border * 2;
  int h = GetMenuSize(20) + border;
  int size = GetMenuSize(13);

  g_editorFont.DrawText2D(x, y += border, size, Strings::Get("dialog_resolution"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_windowed"));
#ifdef HAVE_REFRESH_RATES
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_refreshrate"));
#endif
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_colourdepth"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_zbufferdepth"));
}
