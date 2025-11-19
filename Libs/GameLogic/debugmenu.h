#ifndef _included_debugmenu_h
#define _included_debugmenu_h

class DebugMenu : public GuiWindow
{
  public:
    DebugMenu(std::string_view  name);

    void Create() override;
    void Render(bool hasFocus) override;
};

class DebugKeyBindings
{
  public:
    static void DebugMenu();
    static void NetworkButton();
    static void FPSButton();
#ifdef PROFILER_ENABLED
    static void ProfileButton();
#endif
#ifdef CHEATMENU_ENABLED
    static void CheatButton();
#endif
    static void ReallyQuitButton();
    static void ToggleFullscreenButton();
};

#endif
