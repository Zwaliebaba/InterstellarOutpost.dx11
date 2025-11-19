#ifndef _included_networkwindow_h
#define _included_networkwindow_h

#include "GuiWindow.h"

class NetworkWindow : public GuiWindow
{
  public:
    NetworkWindow(const char* name);

    void Render(bool hasFocus) override;
};

#endif
