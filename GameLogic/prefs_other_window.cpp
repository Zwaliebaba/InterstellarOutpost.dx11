#include "pch.h"
#include "prefs_other_window.h"
#include "GameApp.h"
#include "DropDownMenu.h"

#include "level_file.h"
#include "location.h"
#include "preferences.h"
#include "Renderer.h"
#include "resource.h"
#include "DX9TextRenderer.h"

class ApplyOtherButton : public GuiButton
{
  void MouseUp() override
  {
    auto parent = static_cast<PrefsOtherWindow*>(m_parent);

    if (g_app->m_locationId == -1)
    {
      // Only set the difficulty from the top level
      // Preferences value is 1-based, m_difficultyLevel is 0-based.
      g_prefsManager->SetInt(OTHER_DIFFICULTY, parent->m_difficulty + 1);
      g_app->m_difficultyLevel = parent->m_difficulty;
    }

    if (Location::ChristmasModEnabled())
      g_prefsManager->SetInt(OTHER_CHRISTMASENABLED, parent->m_christmas);

    if (g_app->m_location)
    {
      LandscapeDef* def = &g_app->m_location->m_levelFile->m_landscape;
      g_app->m_location->m_landscape.Init(def);
    }

    bool removeWindows = false;

    g_prefsManager->SetInt(OTHER_AUTOMATICCAM, parent->m_automaticCamera);

    bool oldMode = g_app->m_largeMenus;
    g_prefsManager->SetInt(OTHER_LARGEMENUS, parent->m_largeMenus);
    if (parent->m_largeMenus == 2) // (todo) or is running in media center and tenFootMode == -1
      g_app->m_largeMenus = true;
    else
      g_app->m_largeMenus = false;

    if (oldMode != g_app->m_largeMenus) // tenFootMode option has changed, close all windows
      removeWindows = true;

    if (removeWindows)
    {
      LList<GuiWindow*>* windows = Canvas::EclGetWindows();
      while (windows->Size() > 0)
      {
        GuiWindow* w = windows->GetData(0);
        Canvas::EclRemoveWindow(w->m_name);
      }
    }

    g_prefsManager->Save();
  }
};

PrefsOtherWindow::PrefsOtherWindow()
  : GuiWindow(Strings::Get("dialog_otheroptions"))
{
  SetSize(468, 350);

  SetPosition(ClientEngine::OutputSize().Width / 2 - m_w / 2, ClientEngine::OutputSize().Height / 2 - m_h / 2);

  m_christmas = g_prefsManager->GetInt(OTHER_CHRISTMASENABLED, 1);
  if (g_app->m_locationId == -1)
  {
    m_difficulty = g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
    if (m_difficulty < 0)
      m_difficulty = 0;
  }
  else
    m_difficulty = g_app->m_difficultyLevel;

  m_largeMenus = g_prefsManager->GetInt(OTHER_LARGEMENUS, 0);
  m_automaticCamera = g_prefsManager->GetInt(OTHER_AUTOMATICCAM, 0);
}

void PrefsOtherWindow::Create()
{
  GuiWindow::Create();

  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int x = m_w / 2;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w / 2 - border * 2;
  int h = buttonH + border;
  int fontSize = GetMenuSize(13);

  auto box = new InvertedBox();
  box->SetProperties("invert", 10, y += border, m_w - 20, h * 7 + border * 2);
  RegisterButton(box);

  auto difficulty = new DropDownMenu();
  difficulty->SetProperties(Strings::Get("dialog_difficulty"), x, y += h, buttonW, buttonH);

  for (int i = 0; i < 10; i++)
  {
    char option[32];
    switch (i)
    {
      case 0:
        sprintf(option, "%d (%s)", i + 1, Strings::Get("dialog_standard_difficulty").c_str());
        break;

      case 9:
        sprintf(option, "%d (%s)", i + 1, Strings::Get("dialog_hard_difficulty").c_str());
        break;

      default:
        sprintf(option, "%d", i + 1);
        break;
    }
    difficulty->AddOption(option, i);
  }
  difficulty->RegisterInt(&m_difficulty);
  difficulty->SetDisabled(g_app->m_locationId != -1);
  difficulty->m_fontSize = fontSize;
  RegisterButton(difficulty);
  m_buttonOrder.PutData(difficulty);

  if (Location::ChristmasModEnabled())
  {
    auto christmas = new DropDownMenu();
    christmas->SetProperties(Strings::Get("dialog_christmas"), x, y += h, buttonW, buttonH);
    christmas->AddOption(Strings::Get("dialog_enabled"), 1);
    christmas->AddOption(Strings::Get("dialog_disabled"), 0);
    christmas->RegisterInt(&m_christmas);
    christmas->m_fontSize = fontSize;
    RegisterButton(christmas);
    m_buttonOrder.PutData(christmas);

    box->m_h = h * 8 + border * 2;
    SetSize(390, 380);
  }

  auto tenfoot = new DropDownMenu();
  tenfoot->SetProperties(Strings::Get("dialog_largemenus"), x, y += h, buttonW, buttonH);
  tenfoot->AddOption(Strings::Get("dialog_auto"), 0);
  tenfoot->AddOption(Strings::Get("dialog_disabled"), 1);
  tenfoot->AddOption(Strings::Get("dialog_enabled"), 2);
  tenfoot->RegisterInt(&m_largeMenus);
  tenfoot->m_fontSize = fontSize;
  RegisterButton(tenfoot);
  m_buttonOrder.PutData(tenfoot);

  auto autoCam = new DropDownMenu();
  autoCam->SetProperties(Strings::Get("dialog_autocam"), x, y += h, buttonW, buttonH);
  autoCam->AddOption(Strings::Get("dialog_auto"), 0);
  autoCam->AddOption(Strings::Get("dialog_disabled"), 1);
  autoCam->AddOption(Strings::Get("dialog_enabled"), 2);
  autoCam->RegisterInt(&m_automaticCamera);
  autoCam->m_fontSize = fontSize;
  RegisterButton(autoCam);
  m_buttonOrder.PutData(autoCam);

  y = m_h - h;

  auto cancel = new CloseButton();
  cancel->SetProperties(Strings::Get("dialog_close"), border, y, buttonW, buttonH);
  cancel->m_fontSize = fontSize;
  cancel->m_centered = true;
  RegisterButton(cancel);
  m_buttonOrder.PutData(cancel);

  auto apply = new ApplyOtherButton();
  apply->SetProperties(Strings::Get("dialog_apply"), m_w - buttonW - border, y, buttonW, buttonH);
  apply->m_fontSize = fontSize;
  apply->m_centered = true;
  RegisterButton(apply);
  m_buttonOrder.PutData(apply);
}

void PrefsOtherWindow::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  int border = GetClientRectX1() + 10;
  int size = GetMenuSize(13);
  int x = m_x + 20;
  int y = m_y + GetClientRectY1() + border * 2;
  int h = GetMenuSize(20) + border;

  g_editorFont.DrawText2D(x, y += border, size, Strings::Get("dialog_helpsystem"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_controlhelpsystem"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_bootloaders"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_language"));

  if (g_app->m_locationId != -1)
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_difficulty"));

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  if (Location::ChristmasModEnabled())
    g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_christmas"));

  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_largemenus"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_autocam"));

  g_editorFont.DrawText2DCenter(m_x + m_w / 2.0f, m_y + m_h - GetMenuSize(50), GetMenuSize(15), DARWINIA_VERSION_STRING);
}
