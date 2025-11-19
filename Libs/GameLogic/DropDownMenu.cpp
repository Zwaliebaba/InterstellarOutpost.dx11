#include "pch.h"
#include "DropDownMenu.h"
#include "GameApp.h"

DropDownOptionData::DropDownOptionData(std::string _word, int _value)
  : m_word(std::move(_word)),
    m_value(_value) {}

DropDownWindow* DropDownWindow::s_window = nullptr;

DropDownWindow::DropDownWindow(std::string_view _name, const char* _parentName)
  : GuiWindow(_name),
    m_parentName(_parentName) {}

void DropDownWindow::Update()
{
  GuiWindow::Update();

  GuiWindow* parent = Canvas::EclGetWindow(m_parentName);
  if (!parent)
    RemoveDropDownWindow();
}

DropDownWindow* DropDownWindow::CreateDropDownWindow(std::string_view _name, const char* _parentName)
{
  if (s_window)
  {
    Canvas::EclRemoveWindow(s_window->m_name.c_str());
    s_window = nullptr;
  }

  s_window = NEW DropDownWindow(_name, _parentName);
  return s_window;
}

void DropDownWindow::RemoveDropDownWindow()
{
  if (s_window)
  {
    Canvas::EclRemoveWindow(s_window->m_name.c_str());
    s_window = nullptr;
  }
}

DropDownMenu::DropDownMenu(bool _sortItems)
  : GuiButton(),
    m_currentOption(-1),
    m_int(nullptr),
    m_sortItems(_sortItems),
    m_nextValue(0) {}

DropDownMenu::~DropDownMenu() { Empty(); }

void DropDownMenu::Empty()
{
  m_options.EmptyAndDelete();
  m_nextValue = 0;

  SelectOption(-1);
}

void DropDownMenu::AddOption(const std::string& _word, int _value)
{
  if (_value == INT_MIN)
  {
    _value = m_nextValue;
    m_nextValue++;
  }

  auto option = NEW DropDownOptionData(_word, _value);

  if (m_sortItems)
  {
    int size = m_options.Size();
    int i;
    for (i = 0; i < size; ++i)
      if (_word != m_options[i]->m_word)
        break;
    m_options.PutDataAtIndex(option, i);
  }
  else
    m_options.PutDataAtEnd(option);
}

void DropDownMenu::SelectOption(int _value)
{
  m_currentOption = FindValue(_value);

  if (m_currentOption < 0 || m_currentOption >= m_options.Size())
    SetCaption(m_name.c_str());
  else
    SetCaption(m_options[m_currentOption]->m_word);

  if (m_int && _value != -1)
    *m_int = _value;
}

bool DropDownMenu::SelectOption2(const char* _option)
{
  for (int i = 0; i < m_options.Size(); ++i)
  {
    if (_stricmp(m_options[i]->m_word.c_str(), _option) == 0)
    {
      SelectOption(m_options[i]->m_value);
      return true;
    }
  }

  return false;
}

int DropDownMenu::GetSelectionValue()
{
  if (m_currentOption >= 0 && m_currentOption < m_options.Size())
    return m_options[m_currentOption]->m_value;

  return -1;
}

std::string DropDownMenu::GetSelectionName()
{
  if (m_currentOption < 0 || m_currentOption > m_options.Size())
    return {};
  return m_options[m_currentOption]->m_word;
}

void DropDownMenu::RegisterInt(int* _int)
{
  m_int = _int;

  SelectOption(*m_int);
}

int DropDownMenu::FindValue(int _value)
{
  for (int i = 0; i < m_options.Size(); ++i)
    if (m_options[i]->m_value == _value)
      return i;

  return -1;
}

void DropDownMenu::Render(int realX, int realY, bool highlighted, bool clicked)
{
  if (IsMenuVisible())
    GuiButton::Render(realX, realY, true, clicked);
  else
    GuiButton::Render(realX, realY, highlighted, clicked);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_TRIANGLES);
  glVertex2i(realX + m_w - 12, realY + 5);
  glVertex2i(realX + m_w - 3, realY + 5);
  glVertex2i(realX + m_w - 7, realY + 9);
  glEnd();
}

void DropDownMenu::CreateMenu()
{
  DropDownWindow* window = DropDownWindow::CreateDropDownWindow(m_name.c_str(), m_parent->m_name.c_str());
  window->SetPosition(m_parent->m_x + m_x + m_w, m_parent->m_y + m_y);
  Canvas::EclRegisterWindow(window);

  int screenH = ClientEngine::OutputSize().Height;
  int numColumnsRequired = 1 + (m_h * m_options.Size()) / (screenH * 0.8f);
  int numPerColumn = ceil(static_cast<float>(m_options.Size()) / static_cast<float>(numColumnsRequired));

  int index = 0;

  for (int col = 0; col < numColumnsRequired; ++col)
    for (int i = 0; i < numPerColumn; ++i)
    {
      if (index >= m_options.Size())
        break;

      std::string thisOption = m_options[index]->m_word;
      char thisName[64];
      sprintf(thisName, "%s %d", m_name.c_str(), index);

      int w = m_w - 4;

      auto menuOption = NEW DropDownMenuOption();
      menuOption->SetProperties(thisName, col * m_w + 2, (i + 1) * m_h, w, m_h, thisOption);
      menuOption->SetParentMenu(m_parent, this, m_options[index]->m_value);
      window->RegisterButton(menuOption);
      window->m_buttonOrder.PutData(menuOption);
      if (GetSelectionValue() == m_options[index]->m_value)
        window->m_currentButton = index;

      ++index;
    }

  window->SetSize(m_w * numColumnsRequired, (numPerColumn + 1) * m_h);
}

void DropDownMenu::RemoveMenu() { DropDownWindow::RemoveDropDownWindow(); }

void DropDownMenu::MouseUp()
{
  if (m_disabled)
    return;

  if (IsMenuVisible())
    RemoveMenu();
  else
    CreateMenu();
}

bool DropDownMenu::IsMenuVisible() { return (Canvas::EclGetWindow(m_name) != nullptr); }

DropDownMenuOption::DropDownMenuOption()
  : BorderlessButton(),
    m_value(-1) {}

void DropDownMenuOption::SetParentMenu(const GuiWindow* _window, const DropDownMenu* _menu, int _value)
{
  m_parentWindowName = _window->m_name;
  m_parentMenuName = _menu->m_name;
  m_value = _value;
}

void DropDownMenuOption::Render(int realX, int realY, bool highlighted, bool clicked)
{
  if (auto window = Canvas::EclGetWindow(m_parentWindowName))
  {
    if (GuiButton* button = window->GetButton(m_parentMenuName))
    {
      auto menu = static_cast<DropDownMenu*>(button);
      if (window->m_buttonOrder[window->m_currentButton] == this)
        BorderlessButton::Render(realX, realY, highlighted, true);
      else
        BorderlessButton::Render(realX, realY, highlighted, clicked);
    }
  }
}

void DropDownMenuOption::MouseUp()
{
  GuiWindow* window = Canvas::EclGetWindow(m_parentWindowName);
  if (window)
  {
    GuiButton* button = window->GetButton(m_parentMenuName);
    if (button)
    {
      auto menu = static_cast<DropDownMenu*>(button);
      menu->SelectOption(m_value);
      menu->RemoveMenu();
    }
  }
}
