#include "pch.h"
#include "resource.h"
#include "GameApp.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "filesys_utils.h"
#include "location.h"
#include "shape.h"
#include "sound_stream_decoder.h"
#include "text_stream_readers.h"

void Resource::Shutdown()
{
  FlushOpenGlState();
  m_bitmaps.clear();
  m_shapes.clear();
}

void Resource::AddBitmap(const char* _name, const BitmapRGBA& _bmp, bool _mipMapping)
{
  // Only insert if a bitmap with no other bitmap is already using that name
  if (!m_bitmaps.contains(_name))
  {
    auto bmpCopy = NEW BitmapRGBA(_bmp);
    m_bitmaps.emplace(_name, bmpCopy);
  }
}

TextReader* Resource::GetTextReader(std::string_view _filename)
{
  TextReader* reader = nullptr;
  hstring fullFilename = FileSys::GetHomeDirectory() + to_hstring(_filename);

  if (DoesFileExist(to_string(fullFilename).c_str()))
    reader = NEW TextFileReader(to_string(fullFilename).c_str());

  return reader;
}

BinaryReader* Resource::GetBinaryReader(const char* _filename)
{
  BinaryReader* reader = nullptr;
  std::string fullFilename = to_string(FileSys::GetHomeDirectory()) + _filename;

  if (DoesFileExist(fullFilename.c_str()))
    reader = NEW BinaryFileReader(fullFilename.c_str());

  return reader;
}

int Resource::GetTexture(const char* _name, bool _mipMapping, bool _masked)
{
  int tex;

  // First lookup this name in the BTree of existing textures
  auto it = m_textures.find(_name);

  // If the texture wasn't there, then look in our bitmap store
  bool found = it != m_textures.end();
  if (!found)
  {
    auto bmp = m_bitmaps.find(_name);
    if (bmp != m_bitmaps.end())
    {
      if (_masked)
        bmp->second->ConvertPinkToTransparent();
      tex = bmp->second->ConvertToTexture(_mipMapping);
      m_textures.emplace(_name, tex);
      found = true;
    }
  }
  else
    tex = it->second;

  // If we still didn't find it, try to load it from a file on the disk
  if (!found)
  {
    BinaryReader* reader = GetBinaryReader(_name);

    if (reader)
    {
      const char* extension = GetExtensionPart(_name);
      BitmapRGBA bmp(reader, extension);
      delete reader;

      if (_masked)
        bmp.ConvertPinkToTransparent();
      tex = bmp.ConvertToTexture(_mipMapping);
      m_textures.emplace(_name, tex);
      found = true;
    }
  }

  if (!found)
    Fatal("Failed to load texture {}", _name);

  return tex;
}

bool Resource::DoesTextureExist(const char* _name)
{
  // First lookup this name in the BTree of existing textures
  if (m_textures.contains(_name))
    return true;

  // If the texture wasn't there, then look in our bitmap store
  if (m_bitmaps.contains(_name))
    return true;

  // If we still didn't find it, try to load it from a file on the disk
  char fullPath[512];
  sprintf(fullPath, "%s", _name);
  _strupr(fullPath);
  BinaryReader* reader = GetBinaryReader(fullPath);
  if (reader)
    return true;

  return false;
}

Shape* Resource::GetShape(const char* _name)
{
  Shape* theShape;

  auto it = m_shapes.find(_name);

  // If we haven't loaded the shape before, or _makeNew is true, then
  // try to load it from the disk
  if (it == m_shapes.end())
  {
    theShape = GetShapeCopy(_name, false);
    m_shapes.emplace(_name, theShape);
  }
  else
    theShape = it->second;

  return theShape;
}

Shape* Resource::GetShapeCopy(const char* _name, bool _animating)
{
  Shape* theShape = nullptr;

  hstring fullFilename = FileSys::GetHomeDirectory() + L"shapes\\" + to_hstring(_name);

  if (DoesFileExist(to_string(fullFilename).c_str()))
    theShape = NEW Shape(to_string(fullFilename).c_str(), _animating);

  ASSERT_TEXT(theShape, "Couldn't create shape file {}", _name);
  return theShape;
}

SoundStreamDecoder* Resource::GetSoundStreamDecoder(const char* _filename)
{
  char buf[256];
  sprintf(buf, "%s.wav", _filename);
  BinaryReader* binReader = GetBinaryReader(buf);

  if (!binReader || !binReader->IsOpen())
    return nullptr;

  auto ssd = NEW SoundStreamDecoder(binReader);
  if (!ssd)
    return nullptr;

  return ssd;
}

void Resource::FlushOpenGlState()
{
  // Forget all the texture handles
  m_textures.clear();
}

void Resource::RegenerateOpenGlState()
{
  // Tell the location
  if (g_app->m_location)
    g_app->m_location->RegenerateOpenGlState();
}

// Finds all the filenames in the specified directory that match the specified
// filter. Directory should be like "textures" or "textures\\".
// Filter can be NULL or "" or "*.bmp" or "map_*" or "map_*.txt"
// Set _longResults to true if you want results like "textures\\blah.bmp"
// or false for "blah.bmp"
std::vector<std::string> Resource::ListResources(const char* _dir, const char* _filter, bool _longResults /* = true */)
{
  //
  // List the base data directory

  char fullDirectory[256];
  sprintf(fullDirectory, "%s", _dir);
  auto results = ListDirectory(fullDirectory, _filter, _longResults);

  return results;
}
