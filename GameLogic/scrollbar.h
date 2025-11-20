#pragma once

class ScrollBar
{
  public:
    ScrollBar(GuiWindow* _parent);
    ~ScrollBar() = default;

    void Create(const char* name, int x, int y, int w, int h, int numRows, int winSize, int stepSize = 1);

    void Remove();

    void SetNumRows(int newValue);
    void SetWinSize(int newValue);

    void SetCurrentValue(int newValue);
    void ChangeCurrentValue(int newValue);

  public:
    std::string m_parentWindow;
    std::string m_name;

    int m_x;
    int m_y;
    int m_w;
    int m_h;
    int m_numRows;
    int m_winSize;
    int m_currentValue;
};

class ScrollBarButton : public GuiButton
{
  protected:
    ScrollBar* m_scrollBar;
    int m_grabOffset;

  public:
    ScrollBarButton(ScrollBar* scrollBar);
    void Render(int realX, int realY, bool highlighted, bool clicked) override;
    void MouseUp() override;
    void MouseDown() override;
};

class ScrollChangeButton : public GuiButton
{
  protected:
    ScrollBar* m_scrollBar;
    int m_amount;

  public:
    ScrollChangeButton(ScrollBar* scrollbar, int amount);
    void MouseDown() override;
};
