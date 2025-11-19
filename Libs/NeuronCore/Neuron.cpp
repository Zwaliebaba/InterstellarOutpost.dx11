#include "pch.h"
#include "NetLib.h"

void CoreEngine::Startup()
{
  if (!XMVerifyCPUSupport())
    Fatal(L"CPU does not support the right technology");

  Timer::Core::Startup();
  Tasks::Core::Startup();
  Graphics::Core::Startup();

  NetLib::Startup();
}

void CoreEngine::Shutdown()
{
  NetLib::Shutdown();

  Tasks::Core::Shutdown();
  Graphics::Core::Shutdown();
}
