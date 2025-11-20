#include "pch.h"
#include "GuiButton.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "GuiWindow.h"

GuiButton::GuiButton()
  : m_fontSize(11.0f),
    m_centered(false),
    m_disabled(false),
    m_highlightedThisFrame(false),
    m_mouseHighlightMode(false)
{
  m_name = "New Button";
  GuiButton::SetTooltip(" ");
}

void GuiButton::SetDisabled(bool _disabled) { m_disabled = _disabled; }

void GuiButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
  float y = 7.5f + realY + (m_h - m_fontSize) / 2;

  auto parent = m_parent;

  UpdateButtonHighlight();

  if (!m_mouseHighlightMode)
    highlighted = false;

  if (parent->m_buttonOrder[parent->m_currentButton] == this)
    highlighted = true;

  if (highlighted || clicked)
  {
    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUADS);
    glColor4ub(199, 214, 220, 255);
    if (clicked)
      glColor4ub(255, 255, 255, 255);
    glVertex2f(realX, realY);
    glVertex2f(realX + m_w, realY);
    glColor4ub(112, 141, 168, 255);
    if (clicked)
      glColor4ub(162, 191, 208, 255);
    glVertex2f(realX + m_w, realY + m_h);
    glVertex2f(realX, realY + m_h);
    glEnd();

    glShadeModel(GL_FLAT);

    g_editorFont.SetRenderShadow(true);

    if (m_disabled)
      glColor4ub(128, 128, 75, 30);
    else
      glColor4ub(255, 255, 150, 30);

    if (m_centered)
    {
      g_editorFont.DrawText2DCenter(realX + m_w / 2, y, m_fontSize, m_caption);
      g_editorFont.DrawText2DCenter(realX + m_w / 2, y, m_fontSize, m_caption);
    }
    else
    {
      g_editorFont.DrawText2D(realX + 5, y, m_fontSize, m_caption);
      g_editorFont.DrawText2D(realX + 5, y, m_fontSize, m_caption);
    }
    g_editorFont.SetRenderShadow(false);
  }
  else
  {
    glColor4ub(107, 37, 39, 64);
    //glColor4ub( 82, 56, 102, 64 );
    glBegin(GL_QUADS);
    glVertex2f(realX, realY);
    glVertex2f(realX + m_w, realY);
    glVertex2f(realX + m_w, realY + m_h);
    glVertex2f(realX, realY + m_h);
    glEnd();

    glBegin(GL_LINES);
    glColor4ub(100, 34, 34, 200);
    glVertex2f(realX, realY + m_h);
    glVertex2f(realX, realY);

    glVertex2f(realX, realY);
    glVertex2f(realX + m_w, realY);

    glColor4f(0.1f, 0.0f, 0.0f, 1.0f);
    glVertex2f(realX + m_w, realY);
    glVertex2f(realX + m_w, realY + m_h);

    glVertex2f(realX + m_w, realY + m_h);
    glVertex2f(realX, realY + m_h);
    glEnd();

    if (m_disabled)
      glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    else
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    if (m_centered)
      g_editorFont.DrawText2DCenter(realX + m_w / 2, y, m_fontSize, m_caption);
    else
      g_editorFont.DrawText2D(realX + 5, y, m_fontSize, m_caption);
  }

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GuiButton::SetProperties(const std::string& _name, int _x, int _y, int _w, int _h, const std::string& _caption,
                              const std::string& _tooltip)
{
  if (_w == -1)
    _w = _name.size() * 7 + 9;

  if (_h == -1)
    _h = 15;

  m_name = _name;

  m_x = _x;
  m_y = _y;
  m_w = _w;
  m_h = _h;
  SetCaption(_caption.empty() ? _name : _caption);
  SetTooltip(_tooltip);
}

void GuiButton::UpdateButtonHighlight()
{
  auto parent = m_parent;

  if (parent->m_buttonChangedThisUpdate)
    m_mouseHighlightMode = false;

  if (Canvas::EclMouseInButton(m_parent, this))
  {
    if (!m_highlightedThisFrame)
    {
      parent->SetCurrentButton(this);
      m_highlightedThisFrame = true;
      m_mouseHighlightMode = true;
    }
  }
  else
    m_highlightedThisFrame = false;
}

BorderlessButton::BorderlessButton()
  : GuiButton() {}

void BorderlessButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
  auto parent = m_parent;
  if (parent->m_buttonOrder[parent->m_currentButton] == this)
    clicked = true;
  if (clicked)
  {
    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUADS);
    glColor4ub(199, 214, 220, 255);
    glVertex2f(realX, realY);
    glVertex2f(realX + m_w, realY);
    glColor4ub(112, 141, 168, 255);
    glVertex2f(realX + m_w, realY + m_h);
    glVertex2f(realX, realY + m_h);
    glEnd();
    glShadeModel(GL_FLAT);

    g_editorFont.SetRenderShadow(true);
    glColor4ub(255, 255, 150, 30);
    if (m_centered)
    {
      g_editorFont.DrawText2DCenter(realX + m_w / 2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
      g_editorFont.DrawText2DCenter(realX + m_w / 2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
    }
    else
    {
      g_editorFont.DrawText2D(realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
      g_editorFont.DrawText2D(realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
    }
    g_editorFont.SetRenderShadow(false);
  }
  else
  {
    glColor4ub(107, 37, 39, 64);

    if (highlighted)
    {
      parent->SetCurrentButton(this);
      glBegin(GL_LINES);
      glColor4ub(100, 34, 34, 250);
      glVertex2f(realX, realY + m_h);
      glVertex2f(realX, realY);

      glVertex2f(realX, realY);
      glVertex2f(realX + m_w, realY);

      glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
      glVertex2f(realX + m_w, realY);
      glVertex2f(realX + m_w, realY + m_h);

      glVertex2f(realX + m_w, realY + m_h);
      glVertex2f(realX, realY + m_h);
      glEnd();
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_centered)
      g_editorFont.DrawText2DCenter(realX + m_w / 2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
    else
      g_editorFont.DrawText2D(realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);

    if (highlighted)
    {
      glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
      if (m_centered)
        g_editorFont.DrawText2DCenter(realX + m_w / 2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
      else
        g_editorFont.DrawText2D(realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption);
    }
  }

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  GuiButton::Render(realX, realY, highlighted, clicked);
}

void GameExitButton::MouseUp() { g_app->m_requestQuit = true; }

CloseButton::CloseButton()
  : GuiButton(),
    m_iconised(false) {}

void CloseButton::MouseUp() { Canvas::EclRemoveWindow(m_parent->m_name); }

void CloseButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
  if (m_iconised)
  {
    if (highlighted || clicked)
      glColor4ub(160, 137, 139, 64);
    else
      glColor4ub(60, 37, 39, 64);

    glBegin(GL_QUADS);
    glVertex2f(realX, realY);
    glVertex2f(realX + m_w, realY);
    glVertex2f(realX + m_w, realY + m_h);
    glVertex2f(realX, realY + m_h);
    glEnd();

    glBegin(GL_LINES);
    glColor4ub(0, 0, 150, 100);
    glVertex2f(realX, realY + m_h);
    glVertex2f(realX, realY);

    glVertex2f(realX, realY);
    glVertex2f(realX + m_w, realY);

    glColor4f(0.1f, 0.0f, 0.0f, 1.0f);
    glVertex2f(realX + m_w, realY);
    glVertex2f(realX + m_w, realY + m_h);

    glVertex2f(realX + m_w, realY + m_h);
    glVertex2f(realX, realY + m_h);
    glEnd();
  }
  else
    GuiButton::Render(realX, realY, highlighted, clicked);
}

void LabelButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
  if (m_disabled)
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
  else
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  g_editorFont.DrawText2D(realX + 5, realY + 10, 11.0f, m_caption);
}

void InvertedBox::Render(int realX, int realY, bool highlighted, bool clicked)
{
  //GuiButton::Render( realX, realY, highlighted, clicked );

  glColor4f(0.05f, 0.0f, 0.0f, 0.4f);
  glBegin(GL_QUADS);
  glVertex2f(realX, realY);
  glVertex2f(realX + m_w, realY);
  glVertex2f(realX + m_w, realY + m_h);
  glVertex2f(realX, realY + m_h);
  glEnd();

  //
  // Border lines

  glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
  glBegin(GL_LINES);                        // top
  glVertex2f(realX, realY);
  glVertex2f(realX + m_w, realY);
  glEnd();

  glBegin(GL_LINES);                        // left
  glVertex2f(realX, realY);
  glVertex2f(realX, realY + m_h);
  glEnd();

  glColor4ub(100, 34, 34, 150);
  glBegin(GL_LINES);
  glVertex2f(realX + m_w, realY);
  glVertex2f(realX + m_w, realY + m_h);
  glEnd();

  glBegin(GL_LINES);                        // bottom
  glVertex2f(realX, realY + m_h);
  glVertex2f(realX + m_w, realY + m_h);
  glEnd();
}
