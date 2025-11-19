#pragma once

#include "PixProfiler.h"

namespace Neuron::Graphics
{
  // Controls all the DirectX device resources.
  class Core
  {
    public:
      static constexpr unsigned int AllowTearing = 0x1;
      static constexpr unsigned int EnableHDR = 0x2;

      static void Startup(DXGI_FORMAT _backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
                          DXGI_FORMAT _depthBufferFormat = DXGI_FORMAT_D32_FLOAT, UINT backBufferCount = 2,
                          unsigned int flags = 0) noexcept(false);
      static void Shutdown();
      static bool IsActive() { return sm_isActive; }

      static void CreateDeviceResources();
      static void ReleaseDeviceResources();

      static void ValidateDevice();
      static void HandleDeviceLost();
      static const auto& GetResolutions() { return sm_modeDescs; }

      // Direct3D Accessors.
      static auto* GetDxgiFactory() { return m_dxgiFactory.get(); }

      static DXGI_FORMAT GetBackBufferFormat() { return m_backBufferFormat; }
      static DXGI_FORMAT GetDepthBufferFormat() { return m_depthBufferFormat; }
      static UINT GetCurrentFrameIndex() { return m_backBufferIndex; }
      static UINT GetPreviousFrameIndex() { return m_backBufferIndex == 0 ? m_backBufferCount - 1 : m_backBufferIndex - 1; }
      static UINT GetBackBufferCount() { return m_backBufferCount; }

      static unsigned int GetDeviceOptions() { return m_options; }

    private:
      static void GetAdapter();
      static void CloseAdapter();

      static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

      inline static UINT m_backBufferIndex;

      // Swap chain objects.
      inline static com_ptr<IDXGIFactory7> m_dxgiFactory;

      inline static com_ptr<IDXGIAdapter4> m_dxgiAdapter;
      inline static com_ptr<IDXGIOutput6> m_dxgiOutput;
      inline static std::vector<DXGI_MODE_DESC1> sm_modeDescs;

      // Presentation fence objects.
      inline static UINT64 m_fenceValues[MAX_BACK_BUFFER_COUNT];
      inline static handle m_fenceEvent;

      // Direct3D properties.
      inline static DXGI_FORMAT m_backBufferFormat;
      inline static DXGI_FORMAT m_depthBufferFormat;
      inline static UINT m_backBufferCount;

      inline static DWORD m_dxgiFactoryFlags;

      // Core options (see flags above)
      inline static unsigned int m_options;

      inline static bool sm_isActive = {false};
  };
}
