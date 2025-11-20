#ifndef _included_prefsgraphicswindow_h
#define _included_prefsgraphicswindow_h

#include "GuiWindow.h"

class PrefsGraphicsWindow : public GuiWindow
{
  public:
    int m_landscapeDetail;
    int m_entityDetail;

    PrefsGraphicsWindow();
    ~PrefsGraphicsWindow() override = default;

    void Create() override;
    void Render(bool _hasFocus) override;
};

#endif
