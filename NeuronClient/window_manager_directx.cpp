#include "pch.h"
#include "inputdriver_win32.h"
#include "window_manager.h"

WindowManager* g_windowManager = nullptr;

void Direct3DSwapBuffers();

WindowManager::WindowManager()
  : m_mouseCaptured(false)
{
  DEBUG_ASSERT(g_windowManager == NULL);

  ListAllDisplayModes();
}

// Returns an index into the list of already registered resolutions
int WindowManager::GetResolutionId(int _width, int _height)
{
  for (int i = 0; i < m_resolutions.Size(); ++i)
  {
    Resolution* res = m_resolutions[i];
    if (res->m_width == _width && res->m_height == _height)
      return i;
  }

  return -1;
}

void WindowManager::ListAllDisplayModes()
{
  int i = 0;
  DEVMODE devMode;
  while (EnumDisplaySettings(nullptr, i, &devMode) != 0)
  {
    if (devMode.dmBitsPerPel >= 15 && devMode.dmPelsWidth >= 640 && devMode.dmPelsHeight >= 480)
    {
      int resId = GetResolutionId(devMode.dmPelsWidth, devMode.dmPelsHeight);
      Resolution* res;
      if (resId == -1)
      {
        res = NEW Resolution(devMode.dmPelsWidth, devMode.dmPelsHeight);
        m_resolutions.PutDataAtEnd(res);
      }
      else
        res = m_resolutions[resId];

      if (res->m_refreshRates.FindData(devMode.dmDisplayFrequency) == -1)
        res->m_refreshRates.PutDataAtEnd(devMode.dmDisplayFrequency);
    }
    ++i;
  }
}

Resolution* WindowManager::GetResolution(int _id)
{
  if (m_resolutions.ValidIndex(_id))
    return m_resolutions[_id];

  return nullptr;
}

void WindowManager::Flip() { Direct3DSwapBuffers(); }

void WindowManager::NastyPollForMessages()
{
  MSG msg;
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    // handle or dispatch messages
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void WindowManager::NastySetMousePos(int x, int y)
{
  if (m_mouseOffsetX == INT_MAX)
  {
    auto topRight = ClientEngine::OutputTopLeft();
    m_mouseOffsetX = topRight.X;
    m_mouseOffsetY = topRight.Y;
  }

  SetCursorPos(x + m_mouseOffsetX, y + m_mouseOffsetY);

  extern W32InputDriver* g_win32InputDriver;
  g_win32InputDriver->SetMousePosNoVelocity(x, y);
}

void WindowManager::NastyMoveMouse(int x, int y)
{
  POINT pos;
  GetCursorPos(&pos);
  SetCursorPos(x + pos.x, y + pos.y);

  extern W32InputDriver* g_win32InputDriver;
  g_win32InputDriver->SetMousePosNoVelocity(x, y);
}

void WindowManager::CaptureMouse()
{
  SetCapture(ClientEngine::Window());
  m_mouseCaptured = true;
}

void WindowManager::UncaptureMouse()
{
  ReleaseCapture();
  m_mouseCaptured = false;
}
