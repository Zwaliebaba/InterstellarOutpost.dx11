#include "pch.h"
#include "Display.h"

using namespace Graphics;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;

namespace ScreenRotation
{
  // 0-degree Z-rotation
  static constexpr XMFLOAT4X4 s_rotation0(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);

  // 90-degree Z-rotation
  static constexpr XMFLOAT4X4 s_rotation90(0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f);

  // 180-degree Z-rotation
  static constexpr XMFLOAT4X4 s_rotation180(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f);

  // 270-degree Z-rotation
  static constexpr XMFLOAT4X4 s_rotation270(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f);
}

void Display::Startup()
{
  TextureManager::Startup();
}

void Display::Shutdown()
{
  TextureManager::Shutdown();
}

void Display::CreateDeviceResources() {}
void Display::ReleaseDeviceResources() {}
