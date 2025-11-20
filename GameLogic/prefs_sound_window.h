#ifndef _included_prefssoundwindow_h
#define _included_prefssoundwindow_h

#include "GuiWindow.h"

class PrefsSoundWindow : public GuiWindow
{
  public:
    int m_soundLib;
    int m_mixFreq;
    int m_numChannels;
    int m_useHardware3D;
    int m_swapStereo;
    int m_dspEffects;
    int m_memoryUsage;

    PrefsSoundWindow();

    void Create() override;
    void Render(bool _hasFocus) override;
};

#endif
