#include "pch.h"
#include "InputField.h"
#include "hi_res_time.h"
#include "input.h"
#include "keydefs.h" 
#include "TargetCursor.h"
#include "DX9TextRenderer.h"

#define PIXELS_PER_CHAR	7

InputField::InputField()
  : m_type(TypeNowt),
    m_char(nullptr),
    m_int(nullptr),
    m_float(nullptr),
    m_string(nullptr),
    m_inputBoxWidth(0),
    m_lowBound(0.0f),
    m_highBound(1e4),
    m_callback(nullptr) { m_buf[0] = '\0'; }

void InputField::SetCallback(GuiButton* button) { m_callback = button; }

void InputField::Render(int realX, int realY, bool highlighted, bool clicked)
{
  glColor4f(0.1f, 0.0f, 0.0f, 0.5f);
  float editAreaWidth = m_w * 0.4f;
  glBegin(GL_QUADS);
  glVertex2f(realX + m_w - editAreaWidth, realY);
  glVertex2f(realX + m_w, realY);
  glVertex2f(realX + m_w, realY + m_h);
  glVertex2f(realX + m_w - editAreaWidth, realY + m_h);
  glEnd();

  glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
  glBegin(GL_LINES);                        // top
  glVertex2f(realX + m_w - editAreaWidth, realY);
  glVertex2f(realX + m_w, realY);
  glEnd();

  glBegin(GL_LINES);                        // left
  glVertex2f(realX + m_w - editAreaWidth, realY);
  glVertex2f(realX + m_w - editAreaWidth, realY + m_h);
  glEnd();

  glColor4ub(100, 34, 34, 150);

  glBegin(GL_LINES);
  glVertex2f(realX + m_w, realY);
  glVertex2f(realX + m_w, realY + m_h);
  glEnd();

  glBegin(GL_LINES);                        // bottom
  glVertex2f(realX + m_w - editAreaWidth, realY + m_h);
  glVertex2f(realX + m_w, realY + m_h);
  glEnd();

  int fieldX = realX + m_w - m_inputBoxWidth;

  if (!m_parent->m_currentTextEdit.empty() && (strcmp(m_parent->m_currentTextEdit.c_str(), m_name.c_str()) == 0))
  {
    BorderlessButton::Render(realX, realY, true, clicked);
    if (fmodf(GetHighResTime(), 1.0f) < 0.5f)
    {
      size_t cursorOffset = strlen(m_buf) * PIXELS_PER_CHAR + 2;
      glBegin(GL_LINES);
      glVertex2f(fieldX + static_cast<float>(cursorOffset), realY + 2);
      glVertex2f(fieldX + static_cast<float>(cursorOffset), realY + m_h - 2);
      glEnd();
    }
  }
  else
    BorderlessButton::Render(realX, realY, highlighted, clicked);

  // Edit by Chris - so we don't need to keep Refreshing, just generate the Buffer here every second

  if (!highlighted && fmodf(GetHighResTime(), 1.0f) < 0.5f)
  {
    switch (m_type)
    {
      case TypeChar:
        sprintf(m_buf, "%d", static_cast<int>(*m_char));
        break;
      case TypeInt:
        sprintf(m_buf, "%d", *m_int);
        break;
      case TypeFloat:
        sprintf(m_buf, "%.2f", *m_float);
        break;
      case TypeString:
        strncpy(m_buf, m_string, sizeof(m_buf) - 1);
        break;
    }
    m_inputBoxWidth = strlen(m_buf) * PIXELS_PER_CHAR + 7;
  }
  auto parent = m_parent;
  fieldX = realX + m_w - parent->GetMenuSize(m_inputBoxWidth);
  g_editorFont.DrawText2D(fieldX + 2, realY + 10, parent->GetMenuSize(DEF_FONT_SIZE), m_buf);
}

void InputField::MouseUp() { m_parent->BeginTextEdit(m_name.c_str()); }

void InputField::Keypress(int keyCode, bool shift, bool ctrl, bool alt)
{
  if (strcmp(m_parent->m_currentTextEdit.c_str(), "None") == 0)
    return;

  int len = strlen(m_buf);
  if (keyCode == KEY_BACKSPACE)
  {
    if (len > 0)
      m_buf[len - 1] = '\0';
    if (m_type == TypeString)
    {
      strcpy(m_string, m_buf);
      Refresh();
    }
  }
  else if (keyCode == KEY_ENTER)
  {
    m_parent->EndTextEdit();

    if (m_float)
      *m_float = atof(m_buf);
    else if (m_int)
      *m_int = atoi(m_buf);
    else if (m_string)
      strcpy(m_string, m_buf);
    else if (m_char)
      *m_char = static_cast<unsigned char>(atoi(m_buf));

    ClampToBounds();
    Refresh();
  }
  else if (keyCode == KEY_STOP)
  {
    if (len < sizeof(m_buf) - 1)
      strcat(m_buf, ".");

    if (m_type == TypeString)
    {
      strcpy(m_string, m_buf);
      Refresh();
    }
  }
  else if (keyCode >= KEY_0 && keyCode <= KEY_9)
  {
    if (len < sizeof(m_buf) - 1)
    {
      char buf[2] = " ";
      buf[0] = keyCode & 0xff;
      strcat(m_buf, buf);
    }

    if (m_type == TypeString)
    {
      strcpy(m_string, m_buf);
      Refresh();
    }
  }
  else
  {
    if (m_type == TypeString && keyCode >= KEY_A && keyCode <= KEY_Z)
    {
      unsigned char ascii = keyCode & 0xff;
      if (!shift)
        ascii -= ('A' - 'a');
      int location = strlen(m_buf);
      m_buf[location] = ascii;
      m_buf[location + 1] = '\x0';
    }

    if (m_type == TypeString)
    {
      strcpy(m_string, m_buf);
      Refresh();
    }
  }

  m_inputBoxWidth = strlen(m_buf) * PIXELS_PER_CHAR + 7;
}

void InputField::RegisterChar(unsigned char* _char)
{
  DEBUG_ASSERT(m_type == TypeNowt);
  m_type = TypeChar;
  m_char = _char;
}

void InputField::RegisterInt(int* _int)
{
  DEBUG_ASSERT(m_type == TypeNowt);
  m_type = TypeInt;
  m_int = _int;
}

void InputField::RegisterFloat(float* _float)
{
  DEBUG_ASSERT(m_type == TypeNowt);
  m_type = TypeFloat;
  m_float = _float;
}

void InputField::RegisterString(char* _string)
{
  DEBUG_ASSERT(m_type == TypeNowt);
  m_type = TypeString;
  m_string = _string;
}

void InputField::ClampToBounds()
{
  switch (m_type)
  {
    case TypeChar:
      if (*m_char > m_highBound)
        *m_char = m_highBound;
      else
        if (*m_char < m_lowBound)
          *m_char = m_lowBound;
      break;
    case TypeInt:
      if (*m_int > m_highBound)
        *m_int = m_highBound;
      else
        if (*m_int < m_lowBound)
          *m_int = m_lowBound;
      break;
    case TypeFloat:
      if (*m_float > m_highBound)
        *m_float = m_highBound;
      else
        if (*m_float < m_lowBound)
          *m_float = m_lowBound;
      break;
  }
}

void InputField::Refresh()
{
  switch (m_type)
  {
    case TypeChar:
      sprintf(m_buf, "%d", static_cast<int>(*m_char));
      break;
    case TypeInt:
      sprintf(m_buf, "%d", *m_int);
      break;
    case TypeFloat:
      sprintf(m_buf, "%.2f", *m_float);
      break;
    case TypeString:
      strncpy(m_buf, m_string, sizeof(m_buf) - 1);
      break;
  }

  if (m_callback)
    m_callback->MouseUp();
}

InputScroller::InputScroller()
  : GuiButton(),
    m_inputField(nullptr),
    m_change(0.0f),
    m_mouseDownStartTime(-1.0f) {}

#define INTEGER_INCREMENT_PERIOD 0.1f

void InputScroller::Render(int realX, int realY, bool highlighted, bool clicked)
{
  GuiButton::Render(realX, realY, highlighted, clicked);

  if (m_mouseDownStartTime > 0.0f && m_inputField && g_inputManager->controlEvent(ControlEclipseLMousePressed) && g_target->X() >=
    realX && g_target->X() < realX + m_w && g_target->Y() >= realY && g_target->Y() < realY + m_h)
  {
    float change = m_change;
    if (g_inputManager->controlEvent(ControlScrollSpeedup))
      change *= 5.0f;

    float timeDelta = GetHighResTime() - m_mouseDownStartTime;
    if (m_inputField->m_type == InputField::TypeChar)
    {
      if (timeDelta > INTEGER_INCREMENT_PERIOD)
      {
        *m_inputField->m_char += change;
        m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
      }
    }
    else if (m_inputField->m_type == InputField::TypeInt)
    {
      if (timeDelta > INTEGER_INCREMENT_PERIOD)
      {
        *m_inputField->m_int += change;
        m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
      }
    }
    else if (m_inputField->m_type == InputField::TypeFloat)
    {
      change = change * timeDelta * 0.2f;
      *m_inputField->m_float += change;
    }

    m_inputField->ClampToBounds();
    m_inputField->Refresh();
  }
}

void InputScroller::MouseDown()
{
  if (g_inputManager->controlEvent(ControlEclipseLMousePressed))
    m_mouseDownStartTime = GetHighResTime() - INTEGER_INCREMENT_PERIOD;
}

void InputScroller::MouseUp() { m_mouseDownStartTime = -1.0f; }
