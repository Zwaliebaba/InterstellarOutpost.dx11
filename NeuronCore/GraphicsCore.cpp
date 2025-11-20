#include "pch.h"
#include "GraphicsCore.h"

using namespace Graphics;
using namespace Windows::UI::Core;

namespace
{
  DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt)
  {
    switch (fmt)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
      case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_UNORM;
      default:
        return fmt;
    }
  }
}

// Constructor for Core.
void Core::Startup(const DXGI_FORMAT _backBufferFormat, const DXGI_FORMAT _depthBufferFormat, UINT backBufferCount,
                   unsigned int flags) noexcept(false)
{
  m_backBufferFormat = _backBufferFormat;
  m_depthBufferFormat = _depthBufferFormat;
  m_backBufferCount = backBufferCount;
  m_options = flags;

  if (backBufferCount < 2 || backBufferCount > MAX_BACK_BUFFER_COUNT)
    throw std::out_of_range("invalid backBufferCount");

  CreateDeviceResources();
}

// Destructor for Core.
void Core::Shutdown()
{
  ReleaseDeviceResources();

#ifdef _DEBUG
  {
    com_ptr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_GRAPHICS_PPV_ARGS(dxgiDebug))))
      check_hresult(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));
    else
      DebugTrace(L"Not able to get a Debug Interface");
  }
#endif
}

// Configures the Direct3D device, and stores handles to it and the device context.
void Core::CreateDeviceResources()
{
#if defined(_DEBUG)
  // Enable the debug layer (requires the Graphics Tools "optional feature").
  //
  // NOTE: Enabling the debug layer after device creation will invalidate the active device.
  {
    com_ptr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_GRAPHICS_PPV_ARGS(dxgiInfoQueue))))
    {
      m_dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

      check_hresult(dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true));
      check_hresult(dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true));

      DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
        80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
        820, /* CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE */
        821, /* CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE */
        916, /* REFLECTSHAREDPROPERTIES_INVALIDOBJECT */
        1328, /* CREATERESOURCE_STATE_IGNORED */
      };
      DXGI_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = _countof(hide);
      filter.DenyList.pIDList = hide;
      check_hresult(dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter));
    }
  }
#endif

  check_hresult(CreateDXGIFactory2(m_dxgiFactoryFlags, IID_GRAPHICS_PPV_ARGS(m_dxgiFactory)));

  // Determines whether tearing support is available for fullscreen borderless windows.
  if (m_options & AllowTearing)
  {
    BOOL allowTearing = FALSE;

    HRESULT hr = m_dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

    if (FAILED(hr) || !allowTearing)
    {
      m_options &= ~AllowTearing;
      DebugTrace(L"WARNING: Variable refresh rate displays not supported");
    }
  }

  GetAdapter();

  sm_isActive = true;
}

void Core::ReleaseDeviceResources()
{
  CloseAdapter();

  m_dxgiFactory = nullptr;
  sm_isActive = false;
}

// This method is called in the event handler for the DisplayContentsInvalidated event.
void Core::ValidateDevice()
{
  // The D3D Device is no longer valid if the default adapter changed since the device
  // was created or if the device has been removed.

  DXGI_ADAPTER_DESC previousDesc;
  {
    com_ptr<IDXGIAdapter1> previousDefaultAdapter;
    check_hresult(m_dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.put()));

    check_hresult(previousDefaultAdapter->GetDesc(&previousDesc));
  }

  DXGI_ADAPTER_DESC currentDesc;
  {
    com_ptr<IDXGIFactory7> currentFactory;
    check_hresult(CreateDXGIFactory2(m_dxgiFactoryFlags, IID_GRAPHICS_PPV_ARGS(currentFactory)));

    com_ptr<IDXGIAdapter1> currentDefaultAdapter;
    check_hresult(currentFactory->EnumAdapters1(0, currentDefaultAdapter.put()));

    check_hresult(currentDefaultAdapter->GetDesc(&currentDesc));
  }

  // If the adapter LUIDs don't match, or if the device reports that it has been removed,
  // a new D3D device must be created.

  if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart || previousDesc.AdapterLuid.HighPart != currentDesc.
    AdapterLuid.HighPart)
  {
    DebugTrace("Device Lost on ValidateDevice\n");

    // Create a new device and swap chain.
    HandleDeviceLost();
  }
}

// Recreate all device resources and set them back to the current state.
void Core::HandleDeviceLost()
{
  ReleaseDeviceResources();

#ifdef _DEBUG
  {
    com_ptr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_GRAPHICS_PPV_ARGS(dxgiDebug))))
      check_hresult(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));
    else
      DebugTrace(L"Not able to get a Debug Interface");
  }
#endif

  CreateDeviceResources();
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void Core::GetAdapter()
{
  for (UINT adapterIndex = 0; SUCCEEDED(
         m_dxgiFactory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_GRAPHICS_PPV_ARGS(
           m_dxgiAdapter))); adapterIndex++)
  {
    DXGI_ADAPTER_DESC3 desc;
    check_hresult(m_dxgiAdapter->GetDesc3(&desc));

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      // Don't select the Basic Render Driver adapter.
      continue;

    DebugTrace(L"Direct3D Adapter ({}): VID:{:4X}, PID:{:4X} - {}\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
    break;
  }

  if (!m_dxgiAdapter)
    throw std::exception("No Direct3D 12 device found");

  UINT i = 0;
  com_ptr<IDXGIOutput> pOutput;
  while (m_dxgiAdapter->EnumOutputs(i, pOutput.put()) != DXGI_ERROR_NOT_FOUND)
  {
    DXGI_OUTPUT_DESC desc;
    check_hresult(pOutput->GetDesc(&desc));

    if (desc.AttachedToDesktop)
    {
      pOutput.as(m_dxgiOutput);
      break;
    }
    i++;
  }

  if (m_dxgiOutput)
  {
    UINT num = 0;
    const DXGI_FORMAT format = m_backBufferFormat;
    constexpr UINT FLAGS = 0;
    check_hresult(m_dxgiOutput->GetDisplayModeList1(format, FLAGS, &num, nullptr));

    sm_modeDescs.resize(num);
    check_hresult(m_dxgiOutput->GetDisplayModeList1(format, FLAGS, &num, sm_modeDescs.data()));
  }
}

void Core::CloseAdapter()
{
  m_dxgiOutput = nullptr;
  m_dxgiAdapter = nullptr;
}
