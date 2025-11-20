#include "pch.h"
#include "prefs_keybindings_window.h"
#include "DX9TextRenderer.h"
#include "DropDownMenu.h"
#include "GameApp.h"
#include "control_types.h"
#include "file_paths.h"
#include "main.h"
#include "preferences.h"
#include "Renderer.h"
#include "taskmanager_interface_icons.h"
#include "text_stream_readers.h"

using namespace std;

#define CONTROL_MOUSEBUTTONS "ControlMouseButtons"
#define CONTROL_METHOD "ControlMethod"

struct ControlName
{
  ControlType type;
  bool instant;
  const char* name;
};

static ControlName s_controls[] = {
  ControlCameraLeft,
  false,
  "control_event_left",
  ControlCameraRight,
  false,
  "control_event_right",
  ControlCameraForwards,
  false,
  "control_event_forwards",
  ControlCameraBackwards,
  false,
  "control_event_backwards",
  ControlCameraUp,
  false,
  "control_event_up",
  ControlCameraDown,
  false,
  "control_event_down",
  ControlCameraZoom,
  false,
  "control_event_zoom",
  ControlUnitDeselect,
  true,
  "control_event_deselect",
  ControlIconsChatLog,
  true,
  "control_event_iconschatlog",
  ControlIconsTaskManagerDisplay,
  true,
  "control_event_iconstaskmanagerdisplay",
  ControlIconsTaskManagerEndTask,
  true,
  "control_event_iconstaskmanagerendtask",
  ControlNull,
  false,
  nullptr
};

// These are indexes into the s_controls array of values pertaining to
// gestural and iconic modes respectively, terminated by -1
static int s_gesture_controls[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};
static int s_icon_controls[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, -1};

static int* s_indices = s_gesture_controls;

class RestoreDefaultsButton : public GuiButton
{
  public:
    void MouseUp() override
    {
      TextFileReader reader(InputPrefs::GetSystemPrefsPath());
      if (reader.IsOpen())
      {
        g_inputManager->Clear();
        g_inputManager->parseInputPrefs(reader);
      }

      auto parent = static_cast<PrefsKeybindingsWindow*>(m_parent);
      parent->m_numMouseButtons = g_prefsManager->GetInt(CONTROL_MOUSEBUTTONS, 3);
    }
};

class ApplyKeybindingsButton : public GuiButton
{
  void MouseUp() override
  {
    auto parent = static_cast<PrefsKeybindingsWindow*>(m_parent);
    string key, val;
    PrefsManager prefsMan(InputPrefs::GetUserPrefsPath());
    prefsMan.Clear();

    for (unsigned i = 0; s_controls[i].name != nullptr; ++i)
    {
      g_inputManager->getControlString(s_controls[i].type, key);
      val = parent->m_bindings[i]->pref;
      prefsMan.SetString(key.c_str(), val.c_str());
      g_inputManager->replacePrimaryBinding(s_controls[i].type, val);

      if (ControlIconsTaskManagerDisplay == s_controls[i].type)
      {
        string::size_type pos = val.find("down", 0);
        if (pos != string::npos)
        {
          g_inputManager->getControlString(ControlIconsTaskManagerHide, key);
          val = val.replace(pos, 4, "up", 0, 2);
          prefsMan.SetString(key.c_str(), val.c_str());
          g_inputManager->replacePrimaryBinding(ControlIconsTaskManagerHide, val);
        }
      }
    }

    g_prefsManager->SetInt(CONTROL_MOUSEBUTTONS, parent->m_numMouseButtons);
    g_prefsManager->SetInt(CONTROL_METHOD, parent->m_controlMethod);

    delete g_app->m_taskManagerInterface;
    g_app->m_taskManagerInterface = new TaskManagerInterfaceIcons();

    g_prefsManager->Save();
    prefsMan.Save();
  }
};

class ChangeKeybindingButton : public GuiButton
{
  public:
    int m_id;
    bool m_listening;
    bool m_instant;

    ChangeKeybindingButton(int _id, bool _instant)
      : m_id(_id),
        m_listening(false),
        m_instant(_instant) {}

    void MouseUp() override { m_listening = !m_listening; }

    void Render(int x, int y, bool highlighted, bool clicked) override
    {
      auto parent = static_cast<PrefsKeybindingsWindow*>(m_parent);
      if (m_listening)
      {
        InputSpec spec;
        if (g_inputManager->getFirstActiveInput(spec, m_instant))
        {
          unique_ptr<InputDescription> desc(new InputDescription());
          if (g_inputManager->getInputDescription(spec, *desc))
          {
            parent->m_bindings[m_id] = std::move(desc);
            m_listening = false;
          }
        }
      }

      int time = static_cast<int>(g_gameTime * 3.0f);
      if (!m_listening || time & 1)
      {
        const char* keyName = parent->m_bindings[m_id]->noun.c_str();
        SetCaption(keyName);
      }
      else
        m_caption[0] = '\0';

      GuiButton::Render(x, y, highlighted, clicked);
    }
};

class ControlMethodDropDownMenu : public DropDownMenu
{
  void SelectOption(int _option) override
  {
    auto parent = static_cast<PrefsKeybindingsWindow*>(m_parent);
    if (parent && _option != parent->m_controlMethod)
    {
      parent->Remove();
      // This button now deleted
      parent->m_controlMethod = _option;
      parent->Create();

      auto newMenu = static_cast<DropDownMenu*>(parent->GetButton(Strings::Get("newcontrols_prefsoption")));
      if (newMenu)
        newMenu->SelectOption(_option);
    }
    else
      DropDownMenu::SelectOption(_option);
  }
};

PrefsKeybindingsWindow::PrefsKeybindingsWindow()
  : GuiWindow(Strings::Get("dialog_inputoptions"))
{
  unsigned i;
  for (i = 0; s_controls[i].type != ControlNull; ++i)
  {
    unique_ptr<InputDescription> desc(new InputDescription());
    g_inputManager->getBoundInputDescription(s_controls[i].type, *desc);
    m_bindings.push_back(std::move(desc));
  }

  SetSize(460, 125 + 25 * i);
  SetPosition(ClientEngine::OutputSize().Width / 2 - m_w / 2, ClientEngine::OutputSize().Height / 2 - m_h / 2);

  m_numMouseButtons = g_prefsManager->GetInt(CONTROL_MOUSEBUTTONS, 3);
  m_controlMethod = g_prefsManager->GetInt(CONTROL_METHOD);
}

void PrefsKeybindingsWindow::Create()
{
  GuiWindow::Create();

  int fontSize = GetMenuSize(11);
  int y = GetClientRectY1();
  int border = GetClientRectX1() + GetMenuSize(5);
  int x = m_w * 2 / 3;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - border * 2 - x;
  int h = buttonH + border;

  if (0 == m_controlMethod)
    s_indices = s_gesture_controls;
  else
    s_indices = s_icon_controls;

  auto controlMethod = new ControlMethodDropDownMenu();
  controlMethod->SetProperties(Strings::Get("newcontrols_prefsoption"), x, y += border, buttonW, buttonH);
  controlMethod->AddOption(Strings::Get("newcontrols_prefs_gestures"), 0);
  controlMethod->AddOption(Strings::Get("newcontrols_prefs_icons"), 1);
  controlMethod->RegisterInt(&m_controlMethod);
  controlMethod->m_fontSize = GetMenuSize(11);
  RegisterButton(controlMethod);
  m_buttonOrder.PutData(controlMethod);

  auto box = new InvertedBox();
  unsigned num_controls = 0;
  while (s_indices[num_controls] >= 0)
    num_controls++;
  box->SetProperties("invert", 10, y + h, m_w - 20, (num_controls * h) + border);
  RegisterButton(box);

  y += border;

  for (unsigned j = 0; j < num_controls; ++j)
  {
    int i = s_indices[j];
    auto but = new ChangeKeybindingButton(i, s_controls[i].instant);
    auto eventName = Strings::Get(s_controls[i].name);
    but->SetProperties(eventName, x, y += h, buttonW, buttonH);
    but->m_fontSize = GetMenuSize(15);
    but->m_centered = true;
    auto keyName = m_bindings[i]->noun;
    but->m_caption = keyName;
    RegisterButton(but);
    m_buttonOrder.PutData(but);
  }

  y = m_h - (h + 5);

  auto restore = new RestoreDefaultsButton();
  restore->SetProperties(Strings::Get("dialog_restoredefaults"), border, y - h, m_w - border * 2, buttonH);
  restore->m_fontSize = fontSize;
  restore->m_centered = true;
  RegisterButton(restore);
  m_buttonOrder.PutData(restore);

  int buttonW2 = m_w / 2 - border * 2;

  auto cancel = new CloseButton();
  cancel->SetProperties(Strings::Get("dialog_close"), border, y, buttonW2, buttonH);
  cancel->m_fontSize = fontSize;
  cancel->m_centered = true;
  RegisterButton(cancel);
  m_buttonOrder.PutData(cancel);

  auto apply = new ApplyKeybindingsButton();
  apply->SetProperties(Strings::Get("dialog_apply"), m_w - buttonW2 - border, y, buttonW2, buttonH);
  apply->m_fontSize = fontSize;
  apply->m_centered = true;
  RegisterButton(apply);
  m_buttonOrder.PutData(apply);
}

void PrefsKeybindingsWindow::Remove()
{
  while (m_buttons.Size())
  {
    GuiButton* button = m_buttons[0];
    delete button;
    m_buttons.RemoveData(0);
  }
  m_buttonOrder.Empty();
  m_currentButton = 0;
}

void PrefsKeybindingsWindow::Render(bool _hasFocus)
{
  GuiWindow::Render(_hasFocus);

  int x = m_x + 32;
  int border = GetClientRectX1() + GetMenuSize(5);
  int y = m_y + GetClientRectY1();
  //int y = m_y + GetMenuSize(20);
  int h = GetMenuSize(20) + border;
  int size = GetMenuSize(13);

  g_editorFont.DrawText2D(x, y += 3 * border, size, Strings::Get("newcontrols_prefsoption"));

  for (unsigned j = 0; s_indices[j] >= 0; ++j)
  {
    int i = s_indices[j];
    auto eventName = Strings::Get(s_controls[i].name);
    g_editorFont.DrawText2D(x, y += h, size, eventName);
  }
}
