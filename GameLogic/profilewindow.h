#ifndef _included_profilewindow_h
#define _included_profilewindow_h

#ifdef PROFILER_ENABLED

#include "GuiWindow.h"

class ProfiledElement;

//*****************************************************************************
// Class ProfileWindow
//*****************************************************************************

class ProfileWindow : public GuiWindow
{
  protected:
    int m_yPos;

    void RenderElementProfile(ProfiledElement* _pe, unsigned int _indent);

  public:
    bool m_totalPerSecond;

    ProfileWindow(const char* name);
    ~ProfileWindow() override;

    void Render(bool hasFocus) override;
    void Create() override;
    void Remove() override;
};

#endif // PROFILER_ENABLED

#endif
