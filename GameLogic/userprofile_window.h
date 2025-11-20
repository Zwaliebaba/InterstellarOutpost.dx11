#ifndef _included_userprofilewindow_h
#define _included_userprofilewindow_h

#include "GuiWindow.h"

class UserProfileWindow : public GuiWindow
{
  public:
    UserProfileWindow();

    void Render(bool hasFocus) override;
    void Create() override;
};

class NewUserProfileWindow : public GuiWindow
{
  public:
    static char s_profileName[256];

    NewUserProfileWindow();
    void Create() override;
};

#endif
