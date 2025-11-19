#pragma once

#include "GuiWindow.h"

class DropDownOptionData
{
  public:
    DropDownOptionData(std::string _word, int _value);
    ~DropDownOptionData() = default;

    std::string m_word;
    int m_value;
};

class DropDownWindow : public GuiWindow
{
  public:
    DropDownWindow(std::string_view _name, const char* _parentName);
    void Update() override;

    // There can be only one
    static DropDownWindow* CreateDropDownWindow(std::string_view _name, const char* _parentName);
    static void RemoveDropDownWindow();

  public:
    std::string m_parentName;
    static DropDownWindow* s_window;
};

class DropDownMenu : public GuiButton
{
  protected:
    LList<DropDownOptionData*> m_options;
    int m_currentOption;
    int* m_int;
    bool m_sortItems;
    int m_nextValue;	// Used by AddOption if a value isn't passed in as an argument

  public:
    DropDownMenu(bool _sortItems = false);
    ~DropDownMenu() override;

    void Empty();
    void AddOption(const std::string& _option, int _value = INT_MIN);
    int GetSelectionValue();
    std::string GetSelectionName();
    virtual void SelectOption(int _option);
    bool SelectOption2(const char* _option);

    void CreateMenu();
    void RemoveMenu();
    bool IsMenuVisible();

    void RegisterInt(int* _int);

    int FindValue(int _value);	// Returns an index into m_options

    void Render(int realX, int realY, bool highlighted, bool clicked) override;
    void MouseUp() override;
};

class DropDownMenuOption : public BorderlessButton
{
  public:
    DropDownMenuOption();

    void SetParentMenu(const GuiWindow* _window, const DropDownMenu* _menu, int _value);

    void Render(int realX, int realY, bool highlighted, bool clicked) override;
    void MouseUp() override;

  public:
    std::string m_parentWindowName;
    std::string m_parentMenuName;
    int m_value;			// An integer that the client specifies - often a value from an enum
};
