#ifndef INCLUDED_WINDOW_MANAGER_H
#define INCLUDED_WINDOW_MANAGER_H

#include "llist.h"

// ****************************************************************************
// Class Resolution
// ****************************************************************************

class Resolution
{
public:
  int m_width;
  int m_height;
  LList<int> m_refreshRates;

  Resolution(int _width, int _height)
    : m_width(_width),
    m_height(_height)
  {
  }
};

// ****************************************************************************
// Class WindowManager
// ****************************************************************************

class WindowManager
{
public:
  LList<Resolution*> m_resolutions;
  bool m_invertY; // Whether the Y coordinate needs to be inverted or not.

protected:
  bool m_mouseCaptured;

  int m_mouseOffsetX = { INT_MAX };
  int m_mouseOffsetY = { INT_MAX };

  void ListAllDisplayModes();

public:
  WindowManager();

  int GetResolutionId(int _width, int _height); // Returns -1 if resolution doesn't exist
  Resolution* GetResolution(int _id);

  void Flip();
  void NastyPollForMessages();
  void NastySetMousePos(int x, int y);
  void NastyMoveMouse(int x, int y);

  void CaptureMouse();
  void UncaptureMouse();
};

extern WindowManager* g_windowManager;

#endif
