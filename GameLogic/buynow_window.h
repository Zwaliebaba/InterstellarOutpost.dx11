#pragma once

class BuyNowWindow : public GuiWindow
{
  public:
    BuyNowWindow();

    void Create() override;
    void Render(bool _hasFocus) override;
};
