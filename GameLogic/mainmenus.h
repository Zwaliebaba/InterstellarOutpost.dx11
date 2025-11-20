#ifndef _included_mainmenus_h
#define _included_mainmenus_h

#include "GuiWindow.h"

class MainMenuWindow : public GuiWindow
{
  public:
    MainMenuWindow();

    void Create() override;
    void Render(bool _hasFocus) override;
};

class OptionsMenuWindow : public GuiWindow
{
  public:
    OptionsMenuWindow();

    void Create() override;
};

class LocationWindow : public GuiWindow
{
  public:
    LocationWindow();

    void Create() override;
};

class AboutDarwiniaWindow : public GuiWindow
{
  public:
    AboutDarwiniaWindow();

    void Create() override;
    void Render(bool _hasFocus) override;
};

#endif
