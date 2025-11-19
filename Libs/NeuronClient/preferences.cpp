#include "pch.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GameApp.h"

#include "preferences.h"
#include "resource.h"
#include "text_stream_readers.h"

#include "prefs_other_window.h"

#ifdef TARGET_OS_MACOSX
#include "macosx_hardware_detect.h"
#endif

PrefsManager* g_prefsManager = nullptr;

static bool s_overwrite = false;

// ***************
// Class PrefsItem
// ***************

PrefsItem::PrefsItem()
  : m_int(0) {}

PrefsItem::PrefsItem(char* _line)
{
  // Get key
  char* key = _line;
  while (!isalnum(*key) && *key != '\0')		// Skip leading whitespace
    key++;
  char* c = key;
  while (isalnum(*c))							// Find the end of the key word
    c++;
  *c = '\0';
  m_key = _strupr(key);

  // Get value
  char* value = c + 1;
  while (isspace(*value) || *value == '=')
  {
    if (value == nullptr)
      break;
    value++;
  }

  // Is value a number?
  if (value[0] == '-' || isdigit(value[0]))
  {
    // Guess that number is an int
    m_type = TypeInt;

    // Verify that guess
    c = value;
    int numDots = 0;
    while (*c != '\0')
    {
      if (*c == '.')
        ++numDots;
      ++c;
    }
    if (numDots == 1)
      m_type = TypeFloat;
    else if (numDots > 1)
      m_type = TypeString;

    // Convert string into a real number
    if (m_type == TypeFloat)
      m_float = atof(value);
    else if (m_type == TypeString)
      m_str = _strupr(value);
    else
      m_int = atoi(value);
  }
  else
  {
    m_type = TypeString;
    m_str = _strupr(value);
  }
}

PrefsItem::PrefsItem(const char* _key, const char* _str)
  : m_type(TypeString)
{
  m_key = StrUpr(_key);
  m_str = StrUpr(_str);
}

PrefsItem::PrefsItem(const char* _key, float _float)
  : m_type(TypeFloat),
    m_float(_float) { m_key = StrUpr(_key); }

PrefsItem::PrefsItem(const char* _key, int _int)
  : m_type(TypeInt),
    m_int(_int) { m_key = StrUpr(_key); }

PrefsItem::~PrefsItem()
{
}

PrefsManager::PrefsManager(const std::string& _filename)
{
  m_filename = StrUpr(_filename);
  Load();
}

PrefsManager::~PrefsManager()
{
}

bool PrefsManager::IsLineEmpty(const char* _line)
{
  while (_line[0] != '\0')
  {
    if (_line[0] == '#')
      return true;
    if (isalnum(_line[0]))
      return false;
    ++_line;
  }

  return true;
}

int GetDefaultHelpEnabled()
{
#ifndef DEMOBUILD
  return 1;
#else
	return 0;				// Demo has a tutorial instead
#endif
}

const char* GetDefaultSoundLibrary()
{
#ifdef HAVE_DSOUND
  return "dsound";
#else
	return "software";
#endif
}

int GetDefaultSoundDSP()
{
#ifdef DEMOBUILD
    return 0;
#elif defined(TARGET_OS_MACOSX)
	if (MacOSXSlowCPU())
		return 0;
	else
		return 1;
#else
  return 1;
#endif
}

int GetDefaultSoundChannels()
{
#ifdef TARGET_OS_MACOSX
	if (MacOSXSlowCPU())
		return 16;
	else
		return 32;
#else
  return 32;
#endif
}

int GetDefaultPixelShader()
{
#ifdef TARGET_OS_MACOSX
	// Add call to graphics card check here
	if (MacOSXGraphicsNoAcceleration() || MacOSXGraphicsLowMemory())
		return 0;
	else
		return 1;
#else
  return 1;
#endif
}

int GetDefaultGraphicsDetail()
{
#ifdef TARGET_OS_MACOSX
	if (MacOSXGraphicsNoAcceleration())
		return 3;
	else if (MacOSXGraphicsLowMemory())
		return 2;
	else
		return 1;
#else
  return 1;
#endif
}

void PrefsManager::CreateDefaultValues()
{
  char line[1024];

  AddLine("ServerAddress = 127.0.0.1");
  AddLine("BypassNetwork = 1");
  AddLine("IAmAServer = 1");

  AddLine("\n");

  AddLine("TextLanguage = unknown");

  AddLine("TextSpeed = 15");

  sprintf(line, "HelpEnabled = %d", GetDefaultHelpEnabled());
  AddLine(line);

  AddLine("\n");

  sprintf(line, "SoundLibrary = %s", GetDefaultSoundLibrary());
  AddLine(line);

  AddLine("SoundMixFreq = 22050");
  AddLine("SoundMasterVolume = 255");
  sprintf(line, "SoundChannels = %d", GetDefaultSoundChannels());
  AddLine(line);
  AddLine("SoundHW3D = 0");
  AddLine("SoundSwapStereo = 0");
  AddLine("SoundMemoryUsage = 1");
  AddLine("SoundBufferSize = 512"); // Must be a power of 2 for Linux
  sprintf(line, "SoundDSP = %d", GetDefaultSoundDSP());
  AddLine(line);

  AddLine("\n");

  AddLine("ScreenWindowed = 0");
  AddLine("ScreenZDepth = 24");

  AddLine("\n");

  sprintf(line, "RenderLandscapeDetail = %d", GetDefaultGraphicsDetail());
  AddLine(line);
  sprintf(line, "RenderWaterDetail = %d", GetDefaultGraphicsDetail());
  AddLine(line);
  sprintf(line, "RenderBuildingDetail = %d", GetDefaultGraphicsDetail());
  AddLine(line);
  sprintf(line, "RenderEntityDetail = %d", GetDefaultGraphicsDetail());
  AddLine(line);
  sprintf(line, "RenderCloudDetail = %d", GetDefaultGraphicsDetail());
  AddLine(line);

  sprintf(line, "RenderPixelShader = %d", GetDefaultPixelShader());
  AddLine(line);

  AddLine("\n");

  AddLine("ControlMouseButtons = 3");
  AddLine("ControlMethod = 1");

  AddLine("UserProfile = NewUser");
  AddLine("RenderSpecialLighting = 0");
  AddLine(OTHER_DIFFICULTY " = 1");

  // Override the defaults above with stuff from a default preferences file
  TextReader* reader = Resource::GetTextReader("default_preferences.txt");
  if (reader && reader->IsOpen())
    while (reader->ReadLine())
      AddLine(reader->GetRestOfLine(), true);
}

void PrefsManager::Load(const char* _filename)
{
	std::string fname = m_filename;
  if (_filename != nullptr)
    fname = _filename;

  m_items.clear();

  const hstring fPath = FileSys::GetHomeDirectory() + to_hstring(fname);

  // Try to read preferences if they exist
  FILE* in = fopen(to_string(fPath).c_str(), "r");

  if (!in)
  {
    // Probably first time running the game
    CreateDefaultValues();
  }
  else
  {
    char line[256];
    while (fgets(line, 256, in) != nullptr)
      AddLine(line);
    fclose(in);
  }
}

void PrefsManager::SaveItem(FILE* out, PrefsItem* _item)
{
  switch (_item->m_type)
  {
    case PrefsItem::TypeFloat:
      fprintf(out, "%s = %.2f\n", _item->m_key.c_str(), _item->m_float);
      break;
    case PrefsItem::TypeInt:
      fprintf(out, "%s = %d\n", _item->m_key.c_str(), _item->m_int);
      break;
    case PrefsItem::TypeString:
      fprintf(out, "%s = %s\n", _item->m_key.c_str(), _item->m_str.c_str());
      break;
  }
  _item->m_hasBeenWritten = true;
}

void PrefsManager::Save()
{
  // Doen we even niet
}

void PrefsManager::Clear()
{
  m_items.clear();
  m_fileText.clear();
}

float PrefsManager::GetFloat(const char* _key, float _default) const
{
  auto index = m_items.find(_key);
  if (index == m_items.end())
    return _default;
  PrefsItem* item = index->second;
  if (item->m_type != PrefsItem::TypeFloat)
    return _default;
  return item->m_float;
}

int PrefsManager::GetInt(const char* _key, int _default) const
{
  auto index = m_items.find(_key);
  if (index == m_items.end())
    return _default;
  PrefsItem* item = index->second;
  if (item->m_type != PrefsItem::TypeInt)
    return _default;
  return item->m_int;
}

const char* PrefsManager::GetString(const char* _key, const char* _default) const
{
  auto index = m_items.find(_key);
  if (index == m_items.end())
    return _default;
  PrefsItem* item = index->second;
  if (item->m_type != PrefsItem::TypeString)
    return _default;
  return item->m_str.c_str();
}

void PrefsManager::SetString(const char* _key, const char* _string)
{
  auto index = m_items.find(_key);
  if (index == m_items.end())
  {
    auto item = NEW PrefsItem(_key, _string);
    m_items.emplace(item->m_key, item);
  }
  else
  {
    PrefsItem* item = index->second;
    DEBUG_ASSERT(item->m_type == PrefsItem::TypeString);
    // Note by Chris:
    // The incoming value of _string might also be item->m_str
    // So it is essential to copy _string before freeing item->m_str
    item->m_str = StrUpr(_string);
  }
}

void PrefsManager::SetFloat(const char* _key, float _float)
{
  auto index = m_items.find(_key);
  if (index == m_items.end())
  {
    auto item = NEW PrefsItem(_key, _float);
    m_items.emplace(item->m_key, item);
  }
  else
  {
    PrefsItem* item = index->second;
    DEBUG_ASSERT(item->m_type == PrefsItem::TypeFloat);
    item->m_float = _float;
  }
}

void PrefsManager::SetInt(const char* _key, int _int)
{
  auto index = m_items.find(_key);
  if (index == m_items.end())
  {
    auto item = NEW PrefsItem(_key, _int);
    m_items.emplace(item->m_key, item);
  }
  else
  {
    PrefsItem* item = index->second;
    DEBUG_ASSERT(item->m_type == PrefsItem::TypeInt);
    item->m_int = _int;
  }
}

void PrefsManager::AddLine(const char* _line, bool _overwrite)
{
  if (!_line)
    return;

  bool saveLine = true;

  if (!IsLineEmpty(_line))				// Skip comment lines and blank lines
  {
    auto localCopy = StrUpr(_line);
    char* c = strchr(localCopy.data(), '\n');
    if (c)
      *c = '\0';

    auto item = NEW PrefsItem(localCopy.data());

    auto idx = m_items.find(item->m_key);
    if (_overwrite && idx != m_items.end())
    {
			m_items[item->m_key] = item;
      saveLine = false;
    }

    m_items.emplace(item->m_key, item);
  }

  if (saveLine)
  {
    m_fileText.emplace_back(StrUpr(_line));
  }
}

bool PrefsManager::DoesKeyExist(const char* _key)
{
  return m_items.contains(_key);
}
