#ifndef _included_reallyquit_window_h
#define _included_reallyquit_window_h

#include "GuiWindow.h"

#define REALLYQUIT_WINDOWNAME "Really Quit?"

class ReallyQuitWindow : public GuiWindow
{
  public:
    ReallyQuitWindow();
    void Create() override;
};

#endif // _included_reallyquit_window_h
