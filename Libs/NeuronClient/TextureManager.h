#pragma once

#include <shared_mutex>
#include "ASyncLoader.h"
#include "Color.h"
#include "FileSys.h"
#include "LoaderHelpers.h"

class Bitmap
{
  friend class Texture;

  public:
    Bitmap() = default;
    color_t GetPixel(float _x, float _y) const;
    void LoadFromFile(const std::wstring& _filename);

    auto GetHeader() const { return header; }

    auto GetBitData() const { return bitData; }

    auto GetBitSize() const { return bitSize; }

    auto GetWidth() const { return header->width; }

    auto GetHeight() const { return header->height; }

    byte_buffer_t m_data;

  protected:
    const DDS_HEADER* header = nullptr;
    const uint8_t* bitData = nullptr;
    size_t bitSize = 0;
};

class Texture : public ASyncLoader
{
  friend class TextureManager;

  public:
    Texture() = default;
    ~Texture() = default;

    com_ptr<IDirect3DTexture9> m_texture = {};
    std::unique_ptr<Bitmap> m_bitmap;
    unsigned int m_ref;

  protected:
    size_t m_referenceCount;
    float m_width = {};
    float m_height = {};
};

class TextureManager
{
  public:
    static void Startup() {}
    static void Shutdown();
    static auto GetTextureMap() { return &TextureManager::sm_textures; }

    static std::shared_ptr<Texture> LoadTexture(const std::wstring& _name, bool _keepData = false);
    static void RemoveTexture(const Texture* _texture);

  protected:
    inline static std::shared_mutex sm_mutex;
    inline static std::unordered_map<std::wstring, std::shared_ptr<Texture>> sm_textures;
};

inline auto g_textures = TextureManager::GetTextureMap();
