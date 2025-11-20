#include "pch.h"
#include "GuiWindow.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "InputField.h"
#include "input.h"
#include "resource.h"

GuiWindow::GuiWindow(std::string_view _name)
  : m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_resizable(true),
    m_currentButton(0),
    m_buttonChangedThisUpdate(false),
    m_skipUpdate(false)
{
  SetName(_name);
  SetTitle(_name);
  SetMovable(true);
  m_currentTextEdit = "None";

  _strupr(m_title.data());

  Canvas::EclSetCurrentFocus(m_name);
}

GuiWindow::~GuiWindow()
{
  LList<GuiWindow*>* windows = Canvas::EclGetWindows();
  if (windows->GetData(0))
    Canvas::EclSetCurrentFocus(windows->GetData(0)->m_name);

  while (m_buttons.GetData(0))
  {
    GuiButton* button = m_buttons.GetData(0);
    delete button;
    m_buttons.RemoveData(0);
  }
  m_buttons.Empty();
}

void GuiWindow::CreateValueControl(const std::string& name, int dataType, void* value, int y, float change,
                                   float _lowBound, float _highBound, GuiButton* callback, int x, int w)
{
  if (x == -1)
    x = 10;
  if (w == -1)
    w = m_w - x * 2;

  auto input = NEW InputField();

  if (dataType == InputField::TypeString)
    input->SetProperties(name, x, y, w, GetMenuSize(15));
  else
    input->SetProperties(name, x, y, w - 37, GetMenuSize(15));

  input->m_lowBound = _lowBound;
  input->m_highBound = _highBound;
  input->SetCallback(callback);

  switch (dataType)
  {
    case InputField::TypeChar:
      input->RegisterChar(static_cast<unsigned char*>(value));
      break;
    case InputField::TypeInt:
      input->RegisterInt(static_cast<int*>(value));
      break;
    case InputField::TypeFloat:
      input->RegisterFloat(static_cast<float*>(value));
      break;
    case InputField::TypeString:
      input->RegisterString(static_cast<char*>(value));
      break;
  }
  RegisterButton(input);

  if (dataType != InputField::TypeString)
  {
    char nameLeft[64];
    sprintf(nameLeft, "%s left", name.c_str());
    auto left = NEW InputScroller();
    left->SetProperties(nameLeft, input->m_x + input->m_w + 5, y, 15, 15, "<", "Value left");
    left->m_inputField = input;
    left->m_change = -change;
    RegisterButton(left);

    char nameRight[64];
    sprintf(nameRight, "%s right", name.c_str());
    auto right = NEW InputScroller();
    right->SetProperties(nameRight, input->m_x + input->m_w + 22, y, 15, 15, ">", "Value right");
    right->m_inputField = input;
    right->m_change = change;
    RegisterButton(right);
  }
}

void GuiWindow::RemoveValueControl(char* name)
{
  RemoveButton(name);

  char nameLeft[64];
  sprintf(nameLeft, "%s left", name);
  RemoveButton(nameLeft);

  char nameRight[64];
  sprintf(nameRight, "%s right", name);
  RemoveButton(nameRight);
}

void GuiWindow::Create()
{
  auto close = NEW CloseButton();
  close->SetProperties("Close", m_w - 12, 2, 10, 10, " ", "Close this window");
  close->m_iconised = true;
  RegisterButton(close);
}

void GuiWindow::Remove()
{
  while (m_buttons.Size() > 0)
  {
    GuiButton* button = m_buttons.GetData(0);
    RemoveButton(button->m_name.c_str());
  }
  m_buttonOrder.Empty();
  m_currentButton = 0;
}

// Get the coordinates of the drawable area on the rectangle
int GuiWindow::GetClientRectX1() { return 2; }

int GuiWindow::GetClientRectX2() { return m_w - 2; }

int GuiWindow::GetClientRectY1() { return GetMenuSize(15) + 1; }

int GuiWindow::GetClientRectY2() { return m_h - 2; }

void GuiWindow::Render(bool hasFocus)
{
  //
  // Main body fill

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("Textures\\interface_red.bmp"));
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  float texH = 1.0f;
  float texW = texH * 512.0f / 64.0f;

  glColor4f(1.0f, 1.0f, 1.0f, 0.96f);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(m_x, m_y);
  glTexCoord2f(texW, 0.0f);
  glVertex2f(m_x + m_w, m_y);
  glTexCoord2f(texW, texH);
  glVertex2f(m_x + m_w, m_y + m_h);
  glTexCoord2f(0.0f, texH);
  glVertex2f(m_x, m_y + m_h);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  //
  // Title bar fill

  float titleBarHeight = GetClientRectY1() - 1;
  glShadeModel(GL_SMOOTH);
  glBegin(GL_QUADS);
  glColor4ub(199, 214, 220, 255);
  glVertex2f(m_x, m_y);
  glVertex2f(m_x + m_w, m_y);
  glColor4ub(112, 141, 168, 255);
  glVertex2f(m_x + m_w, m_y + titleBarHeight);
  glVertex2f(m_x, m_y + titleBarHeight);
  glEnd();
  glShadeModel(GL_FLAT);

  glColor4ub(199, 214, 220, 255);
  glBegin(GL_LINES); // top
  glVertex2f(m_x, m_y);
  glVertex2f(m_x + m_w, m_y);
  glEnd();

  glBegin(GL_LINES); // left
  glVertex2f(m_x, m_y);
  glVertex2f(m_x, m_y + m_h);
  glEnd();

  glBegin(GL_LINES);
  glVertex2f(m_x + m_w, m_y); // right
  glVertex2f(m_x + m_w, m_y + m_h);
  glEnd();

  glBegin(GL_LINES); // bottom
  glVertex2f(m_x, m_y + m_h);
  glVertex2f(m_x + m_w, m_y + m_h);
  glEnd();

  glColor4ub(42, 56, 82, 255);
  glBegin(GL_LINE_LOOP);
  glVertex2f(m_x - 2, m_y - 2);
  glVertex2f(m_x + m_w + 1, m_y - 2);
  glVertex2f(m_x + m_w + 1, m_y + m_h + 1);
  glVertex2f(m_x - 2, m_y + m_h + 1);
  glEnd();

  g_gameFont.SetRenderShadow(true);
  glColor4ub(255, 255, 150, 30);
  int y = m_y + 9;
  int fontSize = GetMenuSize(12);
  g_gameFont.DrawText2DCenter(m_x + m_w / 2, y, fontSize, m_title.c_str());
  g_gameFont.DrawText2DCenter(m_x + m_w / 2, y, fontSize, m_title.c_str());
  g_gameFont.SetRenderShadow(false);

  for (int i = m_buttons.Size() - 1; i >= 0; --i)
  {
    GuiButton* button = m_buttons.GetData(i);
    bool highlighted = Canvas::EclMouseInButton(this, button) || strcmp(m_currentTextEdit.c_str(),
                                                                        button->m_name.c_str()) == 0;
    bool clicked = (hasFocus && strcmp(Canvas::EclGetCurrentClickedButton().c_str(), button->m_name.c_str()) == 0);
    button->Render(m_x + button->m_x, m_y + button->m_y, highlighted, clicked);
  }
}

int GuiWindow::GetMenuSize(int _value) { return _value; }

void GuiWindow::Update()
{
  m_buttonChangedThisUpdate = false;
  if (m_skipUpdate)
  {
    m_skipUpdate = false;
    return;
  }

  if (strcmp(Canvas::EclGetCurrentFocus().c_str(), m_name.c_str()) == 0)
  {
    if (g_inputManager->controlEvent(ControlMenuDown))
    {
      m_buttonChangedThisUpdate = true;
      m_currentButton++;
      m_currentButton = std::min(m_currentButton, m_buttonOrder.Size() - 1);
    }
    if (g_inputManager->controlEvent(ControlMenuUp))
    {
      m_buttonChangedThisUpdate = true;
      m_currentButton--;
      m_currentButton = std::max(0, m_currentButton);
    }

    if (g_inputManager->controlEvent(ControlMenuActivate))
    {
      GuiButton* b = m_buttonOrder.GetData(m_currentButton);
      if (b)
        b->MouseUp();
    }

    if (g_inputManager->controlEvent(ControlMenuClose))
      Canvas::EclRemoveWindow(m_name);
  }
}

void GuiWindow::SetCurrentButton(const GuiButton* button)
{
  for (int i = 0; i < m_buttonOrder.Size(); ++i)
  {
    if (m_buttonOrder[i] == button)
    {
      m_currentButton = i;
      return;
    }
  }
}

void GuiWindow::SetPosition(int _x, int _y)
{
  m_x = _x;
  m_y = _y;
}

void GuiWindow::SetSize(int _w, int _h)
{
  m_w = _w;
  m_h = _h;
}

void GuiWindow::SetMovable(bool _movable) { m_movable = _movable; }

void GuiWindow::MakeAllOnScreen()
{
  int screenW = ClientEngine::OutputSize().Width;
  int screenH = ClientEngine::OutputSize().Height;
  if (m_x < 10)
    m_x = 10;
  if (m_y < 10)
    m_y = 10;
  if (m_x + m_w > screenW - 10)
    m_x = screenW - m_w - 10;
  if (m_y + m_h > screenH - 10)
    m_y = screenH - m_h - 10;
}

void GuiWindow::BeginTextEdit(std::string_view _name) { m_currentTextEdit = _name; }

void GuiWindow::EndTextEdit() { m_currentTextEdit = "None"; }

void GuiWindow::RegisterButton(GuiButton* button)
{
  button->SetParent(this);

  m_buttons.PutDataAtStart(button);

  if (button->m_y + button->m_h + 10 > m_h)
  {
    SetSize(m_w, button->m_y + button->m_h + 10);
    MakeAllOnScreen();
  }
}

void GuiWindow::RemoveButton(std::string_view _name)
{
  for (int i = 0; i < m_buttons.Size(); ++i)
  {
    GuiButton* button = m_buttons.GetData(i);
    if (button->m_name == _name)
    {
      m_buttons.RemoveData(i);
      delete button;
    }
  }
}

GuiButton* GuiWindow::GetButton(std::string_view _name)
{
  for (int i = 0; i < m_buttons.Size(); ++i)
  {
    GuiButton* button = m_buttons.GetData(i);
    if (button->m_name == _name)
      return button;
  }

  return nullptr;
}

GuiButton* GuiWindow::GetButton(int _x, int _y)
{
  for (int i = 0; i < m_buttons.Size(); ++i)
  {
    GuiButton* button = m_buttons.GetData(i);

    if (_x >= button->m_x && _x <= button->m_x + button->m_w && _y >= button->m_y && _y <= button->m_y + button->m_h)
      return button;
  }

  return nullptr;
}

void GuiWindow::Keypress(int keyCode, bool shift, bool ctrl, bool alt)
{
  GuiButton* currentTextEdit = GetButton(m_currentTextEdit.c_str());
  if (currentTextEdit)
    currentTextEdit->Keypress(keyCode, shift, ctrl, alt);
}

void GuiWindow::MouseEvent(bool lmb, bool rmb, bool up, bool down) {}
