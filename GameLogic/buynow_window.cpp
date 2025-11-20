#include "pch.h"

#include "resource.h"
#include "DX9TextRenderer.h"
#include "buynow_window.h"
#include "GameApp.h"

#include "preferences.h"
#include "Renderer.h"

class BuyNowButton : public GuiButton
{
  void MouseUp() override
  {
    g_app->m_requestQuit = true;
    Canvas::EclRemoveWindow(m_parent->m_name);
  }
};

BuyNowWindow::BuyNowWindow()
  : GuiWindow(Strings::Get("dialog_buydarwinia"))
{
  int screenW = ClientEngine::OutputSize().Width;
  int screenH = ClientEngine::OutputSize().Height;

  SetSize(370, 150);
  SetPosition(screenW / 2.0f - m_w / 2.0f, screenH / 2.0f - m_h / 2.0f);
}

void BuyNowWindow::Create()
{
  GuiWindow::Create();

  int y = m_h;
  int h = 30;

  GuiButton* close = NEW CloseButton();
  close->SetProperties(Strings::Get("dialog_later"), 10, y -= h, m_w - 20, 20);
  close->m_fontSize = 13;
  close->m_centered = true;
  RegisterButton(close);

  GuiButton* buy = NEW BuyNowButton();
  buy->SetProperties(Strings::Get("dialog_buynow"), 10, y -= h, m_w - 20, 20);
  buy->m_fontSize = 13;
  buy->m_centered = true;
  RegisterButton(buy);
}

void BuyNowWindow::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  int y = m_y + 25;
  int h = 18;

  for (std::array lines = {Strings::Get("dialog_buynow1"), Strings::Get("dialog_buynow2")}; auto& i : lines)
    g_gameFont.DrawText2DCenter(m_x + m_w / 2, y += h, 13, i);
}
