#pragma once

#include "GuiButton.h"

// This widget provides an editable text box where the user can type in numbers
// or short strings

class InputField : public BorderlessButton
{
public:
  char m_buf[256];

  enum
  {
    TypeNowt,
    TypeChar,
    TypeInt,
    TypeFloat,
    TypeString
  };

  int m_type;

  unsigned char* m_char;
  int* m_int;
  float* m_float;
  char* m_string;

  int m_inputBoxWidth;

  float m_lowBound;
  float m_highBound;

  GuiButton* m_callback;

  InputField();

  void Render(int realX, int realY, bool highlighted, bool clicked) override;
  void MouseUp() override;
  void Keypress(int keyCode, bool shift, bool ctrl, bool alt) override;

  // These functions allow you to register a piece of storage with the button, such that
  // the button's value is written back to the storage whenever the user changes the
  // button's value
  void RegisterChar(unsigned char*);
  void RegisterInt(int*);
  void RegisterFloat(float*);
  void RegisterString(char*);

  void ClampToBounds();

  void SetCallback(GuiButton* button); // This button will be clicked on Refresh

  void Refresh(); // Updates the display if the registered variable has changed
};

// ****************************************************************************
// Class InputScroller
// ****************************************************************************

class InputScroller : public GuiButton
{
public:
  InputField* m_inputField;
  float m_change;
  float m_mouseDownStartTime;

  InputScroller();

  void Render(int realX, int realY, bool highlighted, bool clicked) override;
  void MouseDown() override;
  void MouseUp() override;
};
