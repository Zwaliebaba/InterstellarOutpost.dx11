#ifndef INCLUDED_MESSAGE_DIALOG_H
#define INCLUDED_MESSAGE_DIALOG_H

#include "GuiWindow.h"

class MessageDialog : public GuiWindow
{
  protected:
    char* m_messageLines[20];
    int m_numLines;

  public:
    MessageDialog(const char* _name, const char* _message);
    ~MessageDialog() override;

    void Create() override;
    void Render(bool _hasFocus) override;
};

#endif
