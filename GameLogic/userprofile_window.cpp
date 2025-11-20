#include "pch.h"
#include "userprofile_window.h"
#include "GameApp.h"
#include "filesys_utils.h"
#include "global_world.h"
#include "InputField.h"

#include "level_file.h"
#include "location.h"
#include "DX9TextRenderer.h"

class LoadUserProfileButton : public GuiButton
{
  public:
    std::string m_profileName;

    void MouseUp() override
    {
      g_app->SetProfileName(m_profileName.c_str());
      g_app->LoadProfile();
      Canvas::EclRemoveWindow(m_parent->m_name);
      Canvas::EclRemoveWindow(Strings::Get("dialog_mainmenu"));
    }
};

class NewProfileWindowButton : public GuiButton
{
  public:
    void MouseUp() override { Canvas::EclRegisterWindow(new NewUserProfileWindow(), m_parent); }
};

UserProfileWindow::UserProfileWindow()
  : GuiWindow(Strings::Get("dialog_profile")) {}

void UserProfileWindow::Render(bool hasFocus)
{
  GuiWindow::Render(hasFocus);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  g_editorFont.DrawText2DCenter(m_x + m_w / 2, m_y + GetMenuSize(30), GetMenuSize(12), Strings::Get("dialog_currentprofilename"));
  g_editorFont.DrawText2DCenter(m_x + m_w / 2, m_y + GetMenuSize(45), GetMenuSize(16), g_app->m_userProfileName);
}

void UserProfileWindow::Create()
{
  char profileDir[256];
  sprintf(profileDir, "%susers/*.*", g_app->GetProfileDirectory());
  auto profileList = ListSubDirectoryNames(profileDir);
  int numProfiles = profileList.size();

  int windowH = 150 + numProfiles * 30;
  SetSize(300, windowH);
  SetPosition(ClientEngine::OutputSize().Width / 2 - m_w / 2, ClientEngine::OutputSize().Height / 2 - m_h / 2);

  GuiWindow::Create();

  //
  // New profile

  int y = GetMenuSize(50);
  int h = GetMenuSize(30);

  int invertY = y + GetMenuSize(20);

  //
  // Inverted box

  if (numProfiles > 0)
  {
    auto box = new InvertedBox();
    box->SetProperties("Box", 10, invertY, m_w - 20, m_h - invertY - GetMenuSize(60));
    RegisterButton(box);
  }

  //
  // Load existing profile button

  for (auto profileName : profileList)
  {
    char caption[256];
    sprintf(caption, "%s: '%s'", Strings::Get("dialog_loadprofile").c_str(), profileName.c_str());
    auto button = new LoadUserProfileButton();
    button->SetProperties(caption, 20, y += h, m_w - 40, GetMenuSize(20));
    button->m_profileName = profileName;
    button->m_fontSize = GetMenuSize(11);
    button->m_centered = true;
    RegisterButton(button);
    m_buttonOrder.PutData(button);
  }

  auto newProfile = new NewProfileWindowButton();
  newProfile->SetProperties(Strings::Get("dialog_newprofile"), 10, m_h - GetMenuSize(55), m_w - 20, GetMenuSize(20));
  newProfile->m_fontSize = GetMenuSize(13);
  newProfile->m_centered = true;
  RegisterButton(newProfile);
  m_buttonOrder.PutData(newProfile);

  auto cancel = new CloseButton();
  cancel->SetProperties(Strings::Get("dialog_close"), 10, m_h - GetMenuSize(30), m_w - 20, GetMenuSize(20));
  cancel->m_fontSize = GetMenuSize(13);
  cancel->m_centered = true;
  RegisterButton(cancel);
  m_buttonOrder.PutData(cancel);
}

// ============================================================================

class NewProfileButton : public GuiButton
{
  void MouseUp() override
  {
    auto parent = static_cast<NewUserProfileWindow*>(m_parent);
    g_app->SetProfileName(parent->s_profileName);
    g_app->LoadProfile();
    Canvas::EclRemoveWindow(m_parent->m_name);
    Canvas::EclRemoveWindow(Strings::Get("dialog_newprofile"));
    Canvas::EclRemoveWindow(Strings::Get("dialog_mainmenu"));
  }
};

char NewUserProfileWindow::s_profileName[256] = "NewUser";

NewUserProfileWindow::NewUserProfileWindow()
  : GuiWindow(Strings::Get("dialog_newprofile")) {}

void NewUserProfileWindow::Create()
{
  SetSize(300, 110);
  SetPosition(ClientEngine::OutputSize().Width / 2 - m_w / 2, ClientEngine::OutputSize().Height / 2 - m_h / 2);

  GuiWindow::Create();

  auto box = new InvertedBox();
  box->SetProperties("box", 10, GetMenuSize(30), m_w - 20, GetMenuSize(40));
  RegisterButton(box);

  CreateValueControl(Strings::Get("dialog_name"), InputField::TypeString, s_profileName, GetMenuSize(40), 0, 0, 0, nullptr, 20,
                     m_w - 40);

  int y = m_h - GetMenuSize(30);

  auto close = new CloseButton();
  close->SetProperties(Strings::Get("dialog_cancel"), 10, y, m_w / 2 - 15, GetMenuSize(20));
  close->m_fontSize = GetMenuSize(12);
  RegisterButton(close);

  auto newProfile = new NewProfileButton();
  newProfile->SetProperties(Strings::Get("dialog_create"), close->m_x + close->m_w + 10, y, m_w / 2 - 15, GetMenuSize(20));
  newProfile->m_fontSize = GetMenuSize(12);
  RegisterButton(newProfile);
}
