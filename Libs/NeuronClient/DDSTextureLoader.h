#pragma once

namespace DirectX
{
  HRESULT CreateDDSTextureFromBitmap(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_ const Bitmap* bitmap,
                                     _Outptr_ IDirect3DTexture9** texture) noexcept;

  HRESULT CreateDDSTextureFromBitmapEx(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_ const Bitmap* bitmap, _In_ DWORD usage,
                                       _In_ D3DPOOL pool, _Outptr_ IDirect3DTexture9** texture) noexcept;

  // Standard version
  HRESULT CreateDDSTextureFromMemory(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                     _In_ size_t ddsDataSize, _Outptr_ LPDIRECT3DBASETEXTURE9* texture) noexcept;

  // Extended version
  HRESULT CreateDDSTextureFromMemoryEx(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                       _In_ size_t ddsDataSize, _In_ DWORD usage, _In_ D3DPOOL pool,
                                       _Outptr_ LPDIRECT3DBASETEXTURE9* texture) noexcept;

  // Type-specific standard versions
  HRESULT CreateDDSTextureFromMemory(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                     _In_ size_t ddsDataSize, _Outptr_ LPDIRECT3DTEXTURE9* texture) noexcept;

  HRESULT CreateDDSTextureFromMemory(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                     _In_ size_t ddsDataSize, _Outptr_ LPDIRECT3DCUBETEXTURE9* texture) noexcept;

  HRESULT CreateDDSTextureFromMemory(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                     _In_ size_t ddsDataSize, _Outptr_ LPDIRECT3DVOLUMETEXTURE9* texture) noexcept;

  // Type-specific extended versions
  HRESULT CreateDDSTextureFromMemoryEx(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                       _In_ size_t ddsDataSize, _In_ DWORD usage, _In_ D3DPOOL pool,
                                       _Outptr_ LPDIRECT3DTEXTURE9* texture) noexcept;

  HRESULT CreateDDSTextureFromMemoryEx(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                       _In_ size_t ddsDataSize, _In_ DWORD usage, _In_ D3DPOOL pool,
                                       _Outptr_ LPDIRECT3DCUBETEXTURE9* texture) noexcept;

  HRESULT CreateDDSTextureFromMemoryEx(_In_ LPDIRECT3DDEVICE9 d3dDevice, _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                       _In_ size_t ddsDataSize, _In_ DWORD usage, _In_ D3DPOOL pool,
                                       _Outptr_ LPDIRECT3DVOLUMETEXTURE9* texture) noexcept;
}
