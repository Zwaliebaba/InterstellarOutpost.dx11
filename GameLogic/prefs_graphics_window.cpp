#include "pch.h"
#include "prefs_graphics_window.h"
#include "DX9TextRenderer.h"
#include "DropDownMenu.h"
#include "GameApp.h"
#include "level_file.h"
#include "location.h"
#include "preferences.h"
#include "Renderer.h"

#define GRAPHICS_LANDDETAIL         "RenderLandscapeDetail"
#define GRAPHICS_ENTITYDETAIL       "RenderEntityDetail"

class ApplyGraphicsButton : public GuiButton
{
  public:
    void MouseUp() override
    {
      if (g_app->m_location)
      {
        auto parent = static_cast<PrefsGraphicsWindow*>(m_parent);

        g_prefsManager->SetInt(GRAPHICS_LANDDETAIL, parent->m_landscapeDetail);
        g_prefsManager->SetInt(GRAPHICS_ENTITYDETAIL, parent->m_entityDetail);

        LandscapeDef* def = &g_app->m_location->m_levelFile->m_landscape;
        g_app->m_location->m_landscape.Init(def);
      }
    }
};

PrefsGraphicsWindow::PrefsGraphicsWindow()
  : GuiWindow(Strings::Get("dialog_graphicsoptions"))
{
  SetSize(360, 300);
  SetPosition(ClientEngine::OutputSize().Width / 2 - m_w / 2, ClientEngine::OutputSize().Height / 2 - m_h / 2);

  m_landscapeDetail = g_prefsManager->GetInt(GRAPHICS_LANDDETAIL, 1);
  m_entityDetail = g_prefsManager->GetInt(GRAPHICS_ENTITYDETAIL, 1);
}

void PrefsGraphicsWindow::Create()
{
  GuiWindow::Create();

  int fontSize = 11;
  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int x = m_w * 0.6;
  int buttonH = 20;
  int buttonW = m_w - x - border * 2;
  int buttonW2 = m_w / 2 - border * 2;
  int h = buttonH + border;

  auto box = NEW InvertedBox();
  box->SetProperties("invert", 10, y + border / 2, m_w - 20, GetMenuSize(190));
  RegisterButton(box);

  auto landDetail = NEW DropDownMenu();
  landDetail->SetProperties(Strings::Get("dialog_landscapedetail"), x, y += border, buttonW, buttonH);
  landDetail->AddOption(Strings::Get("dialog_high"), 1);
  landDetail->AddOption(Strings::Get("dialog_medium"), 2);
  landDetail->AddOption(Strings::Get("dialog_low"), 3);
  landDetail->AddOption(Strings::Get("dialog_upgrade"), 4);
  landDetail->RegisterInt(&m_landscapeDetail);
  landDetail->m_fontSize = fontSize;
  RegisterButton(landDetail);
  m_buttonOrder.PutData(landDetail);

  auto entityDetail = NEW DropDownMenu();
  entityDetail->SetProperties(Strings::Get("dialog_entitydetail"), x, y += h, buttonW, buttonH);
  entityDetail->AddOption(Strings::Get("dialog_high"), 1);
  entityDetail->AddOption(Strings::Get("dialog_medium"), 2);
  entityDetail->AddOption(Strings::Get("dialog_low"), 3);
  entityDetail->RegisterInt(&m_entityDetail);
  entityDetail->m_fontSize = fontSize;
  RegisterButton(entityDetail);
  m_buttonOrder.PutData(entityDetail);

  auto cancel = NEW CloseButton();
  cancel->SetProperties(Strings::Get("dialog_close"), border, m_h - h, buttonW2, buttonH);
  cancel->m_fontSize = GetMenuSize(13);
  cancel->m_centered = true;
  RegisterButton(cancel);
  m_buttonOrder.PutData(cancel);

  auto apply = NEW ApplyGraphicsButton();
  apply->SetProperties(Strings::Get("dialog_apply"), m_w - buttonW2 - border, m_h - h, buttonW2, buttonH);
  apply->m_fontSize = GetMenuSize(13);
  apply->m_centered = true;
  RegisterButton(apply);
  m_buttonOrder.PutData(apply);
}

void PrefsGraphicsWindow::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  int border = GetClientRectX1() + 10;
  int h = GetMenuSize(20) + border;
  int x = m_x + 20;
  int y = m_y + GetClientRectY1() + border;
  int size = GetMenuSize(13);
  LList<char*> elements;

  g_editorFont.DrawText2D(x, y += border, size, Strings::Get("dialog_landscapedetail"));
  g_editorFont.DrawText2D(x, y += h, size, Strings::Get("dialog_entitydetail"));

  char fpsCaption[64];
  sprintf(fpsCaption, "%d FPS", g_app->m_renderer->m_fps);
  g_editorFont.DrawText2DCenter(m_x + m_w / 2, m_y + m_h - GetMenuSize(60), GetMenuSize(20), fpsCaption);
}
