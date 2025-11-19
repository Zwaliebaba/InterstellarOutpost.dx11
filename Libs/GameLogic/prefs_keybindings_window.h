#ifndef INCLUDED_KEYBINDINGS_WINDOW_H
#define INCLUDED_KEYBINDINGS_WINDOW_H

#include "auto_vector.h"
#include "GuiWindow.h"
#include "input.h"

using InputDescList = auto_vector<InputDescription>;

class PrefsKeybindingsWindow : public GuiWindow
{
  public:
    InputDescList m_bindings;
    int m_numMouseButtons;
    int m_controlMethod;

    PrefsKeybindingsWindow();

    void Create() override;
    void Remove() override;

    void Render(bool _hasFocus) override;
};

#endif
