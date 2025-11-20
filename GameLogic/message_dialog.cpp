#include "pch.h"
#include "message_dialog.h"
#include "DX9TextRenderer.h"
#include "Renderer.h"

class OKButton : public GuiButton
{
  protected:
    MessageDialog* m_parent;

  public:
    OKButton(MessageDialog* _parent)
      : m_parent(_parent) {}

    void MouseUp() override { Canvas::EclRemoveWindow(m_parent->m_name); }
};

MessageDialog::MessageDialog(const char* _name, const char* _message)
  : GuiWindow(_name),
    m_numLines(0)
{
  const char* lineStart = _message;
  int longestLine = 0;
  while (true)
  {
    if (_message[0] == '\n' || _message[0] == '\0')
    {
      int lineLen = _message - lineStart;
      if (lineLen > longestLine)
        longestLine = lineLen;
      m_messageLines[m_numLines] = NEW char [lineLen + 1];
      strncpy(m_messageLines[m_numLines], lineStart, lineLen);
      m_messageLines[m_numLines][lineLen] = '\0';
      m_numLines++;
      lineStart = _message + 1;
    }

    if (_message[0] == '\0')
      break;

    _message++;
  }

  m_w = g_editorFont.GetTextWidth(longestLine) + 10;
  m_w = std::max(m_w, 100.0f);
  m_h = 65 + m_numLines * DEF_FONT_SIZE;
  m_h = std::max(m_h, 85.0f);
  SetSize(m_w, m_h);
  m_x = ClientEngine::OutputSize().Width / 2 - m_w / 2;
  m_y = ClientEngine::OutputSize().Height / 2 - m_h / 2;
}

MessageDialog::~MessageDialog()
{
  for (int i = 0; i < m_numLines; ++i)
    delete [] m_messageLines[i];
}

void MessageDialog::Create()
{
  const int buttonWidth = GetMenuSize(40);
  const int buttonHeight = GetMenuSize(18);
  auto button = NEW OKButton(this);

  button->SetProperties(Strings::Get("dialog_close"), (m_w - buttonWidth) / 2, m_h - GetMenuSize(30), buttonWidth, buttonHeight);
  button->m_fontSize = GetMenuSize(11);
  RegisterButton(button);
}

void MessageDialog::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  for (int i = 0; i < m_numLines; ++i)
  {
    g_editorFont.DrawText2D(m_x + 10, m_y + GetMenuSize(32) + i * GetMenuSize(DEF_FONT_SIZE), GetMenuSize(DEF_FONT_SIZE),
                            m_messageLines[i]);
  }
}
