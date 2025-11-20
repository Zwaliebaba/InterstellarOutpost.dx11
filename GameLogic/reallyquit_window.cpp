#include "pch.h"

#include "reallyquit_window.h"
#include "GameApp.h"
#include "Renderer.h"

ReallyQuitWindow::ReallyQuitWindow()
  : GuiWindow(REALLYQUIT_WINDOWNAME)
{
  m_w = 160;
  m_h = 90;
  m_x = ClientEngine::OutputSize().Width / 2 - m_w / 2;
  m_y = ClientEngine::OutputSize().Height / 2 - m_h / 2;
}

void ReallyQuitWindow::Create()
{
  GuiWindow::Create();

  int y = 0, h = 30;

  GuiButton* exit = new GameExitButton();
  exit->SetProperties("Leave Darwinia", 10, y += h, m_w - 20, 20);
  exit->m_fontSize = 13;
  exit->m_centered = true;
  RegisterButton(exit);

  GuiButton* close = new CloseButton();
  close->SetProperties("No. Play On!", 10, y += h, m_w - 20, 20);
  close->m_fontSize = 13;
  close->m_centered = true;
  RegisterButton(close);
}
