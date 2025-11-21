#include <windows.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Voxel
{
    XMFLOAT3 center;
    XMFLOAT3 halfSize;
    XMFLOAT4 rotation; // quaternion
    XMFLOAT4 color;
};

struct SceneConstants
{
    XMMATRIX view;
    XMMATRIX proj;
    XMMATRIX invView;
    XMMATRIX invProj;
    XMFLOAT3 cameraPos;
    float pad0;
    XMFLOAT3 lightDir;
    float pad1;
    XMFLOAT2 viewportSize;
    XMFLOAT2 pad2;
};

struct Vertex
{
    XMFLOAT2 pos;
};

HWND g_hWnd = nullptr;
UINT g_width = 1280;
UINT g_height = 720;

ComPtr<ID3D11Device> g_device;
ComPtr<ID3D11DeviceContext> g_context;
ComPtr<IDXGISwapChain> g_swapChain;
ComPtr<ID3D11RenderTargetView> g_rtv;
ComPtr<ID3D11DepthStencilView> g_dsv;
ComPtr<ID3D11Texture2D> g_depth;
ComPtr<ID3D11Buffer> g_vb;
ComPtr<ID3D11Buffer> g_cb;
ComPtr<ID3D11Buffer> g_voxelBuffer;
ComPtr<ID3D11ShaderResourceView> g_voxelSRV;
ComPtr<ID3D11InputLayout> g_layout;
ComPtr<ID3D11VertexShader> g_vs;
ComPtr<ID3D11PixelShader> g_ps;

std::vector<Voxel> g_voxels;
SceneConstants g_constants{};

bool InitWindow(HINSTANCE hInstance, int nCmdShow);
bool InitD3D();
void Cleanup();
void Render();
void Resize(UINT width, UINT height);
void UpdateConstants();
void CreateScene();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    if (!InitWindow(hInstance, nCmdShow))
        return 0;
    if (!InitD3D())
        return 0;

    CreateScene();

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    Cleanup();
    return static_cast<int>(msg.wParam);
}

bool InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"VoxelWin32";

    if (!RegisterClassEx(&wc))
        return false;

    RECT rc = { 0, 0, static_cast<LONG>(g_width), static_cast<LONG>(g_height) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindow(wc.lpszClassName, L"Voxel Renderer DX11", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd)
        return false;

    ShowWindow(g_hWnd, nCmdShow);
    return true;
}

bool InitD3D()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_width;
    sd.BufferDesc.Height = g_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevels, 1, D3D11_SDK_VERSION, &sd, &g_swapChain,
        &g_device, &obtained, &g_context);
    if (FAILED(hr)) return false;

    Resize(g_width, g_height);

    // Quad vertex buffer
    Vertex vertices[4] = {
        { XMFLOAT2(-1.f, -1.f) },
        { XMFLOAT2(-1.f,  1.f) },
        { XMFLOAT2( 1.f, -1.f) },
        { XMFLOAT2( 1.f,  1.f) },
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices;
    g_device->CreateBuffer(&vbDesc, &vbData, &g_vb);

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = sizeof(SceneConstants);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;
    g_device->CreateBuffer(&cbDesc, nullptr, &g_cb);

    // Compile shaders
    ComPtr<ID3DBlob> vsBlob, psBlob, errors;
    hr = D3DCompileFromFile(L"VoxelShaders.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errors);
    if (FAILED(hr)) return false;
    hr = D3DCompileFromFile(L"VoxelShaders.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errors);
    if (FAILED(hr)) return false;

    g_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vs);
    g_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_ps);

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_device->CreateInputLayout(layoutDesc, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_layout);

    return true;
}

void CreateScene()
{
    // Simple grid of voxels
    g_voxels.clear();
    for (int z = -2; z <= 2; ++z)
    {
        for (int x = -2; x <= 2; ++x)
        {
            Voxel v{};
            v.center = XMFLOAT3(static_cast<float>(x), 0.0f, static_cast<float>(z));
            v.halfSize = XMFLOAT3(0.4f, 0.4f, 0.4f);
            v.rotation = XMFLOAT4(0, 0, 0, 1); // identity quaternion
            float r = (x + 2) / 4.0f;
            float g = (z + 2) / 4.0f;
            v.color = XMFLOAT4(r, g, 1.0f - r * 0.5f, 1.0f);
            g_voxels.push_back(v);
        }
    }

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(g_voxels.size() * sizeof(Voxel));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(Voxel);

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = g_voxels.data();
    g_device->CreateBuffer(&desc, &initData, &g_voxelBuffer);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(g_voxels.size());
    g_device->CreateShaderResourceView(g_voxelBuffer.Get(), &srvDesc, &g_voxelSRV);
}

void Cleanup()
{
    if (g_context) g_context->ClearState();
}

void Resize(UINT width, UINT height)
{
    g_width = width;
    g_height = height;

    if (g_context) g_context->OMSetRenderTargets(0, nullptr, nullptr);
    g_rtv.Reset();
    g_dsv.Reset();
    g_depth.Reset();

    if (g_swapChain)
    {
        g_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &g_rtv);

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    g_device->CreateTexture2D(&depthDesc, nullptr, &g_depth);
    g_device->CreateDepthStencilView(g_depth.Get(), nullptr, &g_dsv);

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<FLOAT>(width);
    vp.Height = static_cast<FLOAT>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_context->RSSetViewports(1, &vp);
}

void UpdateConstants()
{
    static float angle = 0.0f;
    angle += 0.25f * 3.14159f / 180.0f; // slow orbit

    XMVECTOR eye = XMVectorSet(sinf(angle) * 6.0f, 3.0f, cosf(angle) * 6.0f, 0.0f);
    XMVECTOR at = XMVectorZero();
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<float>(g_width) / g_height, 0.1f, 100.0f);

    g_constants.view = XMMatrixTranspose(view);
    g_constants.proj = XMMatrixTranspose(proj);
    g_constants.invView = XMMatrixTranspose(XMMatrixInverse(nullptr, view));
    g_constants.invProj = XMMatrixTranspose(XMMatrixInverse(nullptr, proj));
    XMStoreFloat3(&g_constants.cameraPos, eye);
    g_constants.lightDir = XMFLOAT3(0.3f, -1.0f, 0.2f);
    g_constants.viewportSize = XMFLOAT2(static_cast<float>(g_width), static_cast<float>(g_height));

    g_context->UpdateSubresource(g_cb.Get(), 0, nullptr, &g_constants, 0, 0);
}

void Render()
{
    UpdateConstants();

    const float clearColor[4] = { 0.05f, 0.07f, 0.1f, 1.0f };
    g_context->ClearRenderTargetView(g_rtv.Get(), clearColor);
    g_context->ClearDepthStencilView(g_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    g_context->OMSetRenderTargets(1, g_rtv.GetAddressOf(), g_dsv.Get());

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_context->IASetVertexBuffers(0, 1, g_vb.GetAddressOf(), &stride, &offset);
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    g_context->IASetInputLayout(g_layout.Get());

    g_context->VSSetShader(g_vs.Get(), nullptr, 0);
    g_context->PSSetShader(g_ps.Get(), nullptr, 0);

    g_context->VSSetConstantBuffers(0, 1, g_cb.GetAddressOf());
    g_context->PSSetConstantBuffers(0, 1, g_cb.GetAddressOf());
    g_context->VSSetShaderResources(0, 1, g_voxelSRV.GetAddressOf());
    g_context->PSSetShaderResources(0, 1, g_voxelSRV.GetAddressOf());

    g_context->DrawInstanced(4, static_cast<UINT>(g_voxels.size()), 0, 0);

    g_swapChain->Present(1, 0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (g_device)
        {
            UINT w = LOWORD(lParam);
            UINT h = HIWORD(lParam);
            Resize(w, h);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
