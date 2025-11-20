#pragma once

#include "Color.h"
#include "TextureManager.h"

class Display
{
	public:
		static void Startup();
		static void Shutdown();

    static void CreateDeviceResources();
    static void ReleaseDeviceResources();

    // Device Accessors.
    static Windows::Foundation::Size GetOutputSize() { return m_outputSize; }
    static DXGI_MODE_ROTATION GetRotation() { return m_rotation; }
    static float GetDpi() { return sm_dpi; }
    static float GetAspectRatio() { return m_aspectRatio; }

  protected:
    inline static DXGI_MODE_ROTATION m_rotation{ DXGI_MODE_ROTATION_IDENTITY };
    inline static Windows::Foundation::Size m_outputSize;
    inline static float m_aspectRatio;

    inline static float sm_dpi{ -1.0f };

    inline static XMFLOAT4X4 m_orientationTransform3D;
};

