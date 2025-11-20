#ifndef USER_INPUT_H
#define USER_INPUT_H

#include "LegacyVector3.h"
#include "llist.h"

#include "worldobject.h"

class StretchyIcons;
class Building;
class Engineer;

class UserInput
{
  public:
    bool m_removeTopLevelMenu;

  private:
    LegacyVector3 m_mousePos3d;

    void AdvanceMouse();
    void AdvanceMenus();

    LList<LegacyVector3*> m_mousePosHistory;

  public:
    UserInput();
    void Advance();
    void Render();

    void RecalcMousePos3d();			// Updates the cached value of m_mousePos3d by doing a ray cast against landscape
    LegacyVector3 GetMousePos3d();            // Returns the cached value "m_mousePos3d"
};

#endif
