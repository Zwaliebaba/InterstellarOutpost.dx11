#ifndef _included_prefsotherwindow_h
#define _included_prefsotherwindow_h

class PrefsOtherWindow : public GuiWindow
{
  public:
    int m_christmas;
    int m_difficulty;
    int m_largeMenus;
    int m_automaticCamera;

    PrefsOtherWindow();

    void Create() override;
    void Render(bool _hasFocus) override;
};

// Defines useful to reference preferences from
// other parts of the program.
#define OTHER_CHRISTMASENABLED  "ChristmasEnabled"
#define OTHER_LANGUAGE          "TextLanguage"
#define OTHER_DIFFICULTY		"Difficulty"
#define OTHER_LARGEMENUS		"LargeMenus"
#define OTHER_AUTOMATICCAM      "AutomaticCamera"

#endif
