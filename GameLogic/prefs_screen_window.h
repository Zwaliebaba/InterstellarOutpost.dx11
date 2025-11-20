#ifndef _included_prefsscreenwindow_h
#define _included_prefsscreenwindow_h

#include "GuiWindow.h"

class PrefsScreenWindow : public GuiWindow
{
  public:
    int m_resId;
    int m_windowed;
    int m_colourDepth;
    int m_refreshRate;
    int m_zDepth;

    PrefsScreenWindow();

    void Create() override;
    void Render(bool _hasFocus) override;
};

void SetWindowed(bool _isWindowed, bool _isPermanent, bool& _isSwitchingWindowed);

#endif
