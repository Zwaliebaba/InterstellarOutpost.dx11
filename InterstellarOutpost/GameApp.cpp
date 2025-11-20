#include "pch.h"
#include "GameApp.h"
#include "DX9TextRenderer.h"
#include "NetworkClient.h"
#include "camera.h"
#include "gamecursor.h"
#include "global_world.h"
#include "location.h"
#include "location_input.h"
#include "ParticleSystem.h"
#include "preferences.h"
#include "prefs_other_window.h"
#include "profiler.h"
#include "Renderer.h"
#include "Script.h"
#include "soundsystem.h"
#include "taskmanager.h"
#include "taskmanager_interface_icons.h"
#include "user_input.h"

GameApp* g_app = nullptr;

#define GAMEDATAFILE "game.txt"

namespace
{
  const std::vector<std::wstring> g_supportedLanguages =
  {
 L"ENG", L"FRA", L"ITA", L"DEU", L"ESP", L"RUS", L"POL", L"POR", L"JPN", L"KOR", L"CHS", L"CHT"
  };
}

GameApp::GameApp()
  : m_userInput(nullptr),
    m_soundSystem(nullptr),
    m_particleSystem(nullptr),
    m_profiler(nullptr),
    m_globalWorld(nullptr),
    m_location(nullptr),
    m_locationId(-1),
    m_camera(nullptr),
    m_networkClient(nullptr),
    m_renderer(nullptr),
    m_locationInput(nullptr),
    m_taskManager(nullptr),
    m_script(nullptr),
    m_startSequence(nullptr),
    m_difficultyLevel(0),
    m_largeMenus(false),
    m_paused(false),
    m_requestedLocationId(-1),
    m_requestQuit(false),
    m_levelReset(false),
    m_atMainMenu(false),
    m_gameMode(GameModeNone)
{
  g_app = this;

  g_prefsManager = NEW PrefsManager(GetPreferencesPath());

  m_backgroundColor.Set(0, 0, 0, 0);

  UpdateDifficultyFromPreferences();

#ifdef PROFILER_ENABLED
  m_profiler = NEW Profiler();
#endif

  m_renderer = NEW Renderer();
  m_renderer->Initialize();

  m_gameCursor = NEW GameCursor();
  m_soundSystem = NEW SoundSystem();
  m_networkClient = NEW NetworkClient();
  m_userInput = NEW UserInput();

  m_camera = NEW Camera();

  m_gameDataFile = "game.txt";

  // Determine default language if possible
  auto isoLang = Windows::System::UserProfile::GlobalizationPreferences::Languages().GetAt(0);
  auto lang = std::wstring(Windows::Globalization::Language(isoLang).AbbreviatedName());
  auto country = Windows::Globalization::GeographicRegion().DisplayName();
  m_isoLang = to_string(isoLang);

  Strings::Load(std::format(L"Strings-{}.json", lang));

  SetLanguage(lang);

  SetProfileName(g_prefsManager->GetString("UserProfile", "none"));

  m_particleSystem = NEW ParticleSystem();
  m_taskManager = NEW TaskManager();
  m_script = NEW Script();

  m_taskManagerInterface = NEW TaskManagerInterfaceIcons();

  m_soundSystem->Initialize();

  int menuOption = g_prefsManager->GetInt(OTHER_LARGEMENUS, 0);
  if (menuOption == 2) // (todo) or is running in media center and tenFootMode == -1
    m_largeMenus = true;

  //
  // Load save games

  std::ignore = LoadProfile();
}

GameApp::~GameApp()
{
  SAFE_DELETE(m_globalWorld);
  SAFE_DELETE(m_taskManagerInterface);
  SAFE_DELETE(m_script);
  SAFE_DELETE(m_taskManager);
  SAFE_DELETE(m_particleSystem);
  SAFE_DELETE(m_camera);
  SAFE_DELETE(m_userInput);
  SAFE_DELETE(m_networkClient);
  SAFE_DELETE(m_soundSystem);
  SAFE_DELETE(m_gameCursor);
  SAFE_DELETE(m_renderer);
#ifdef PROFILER_ENABLED
  SAFE_DELETE(m_profiler);
#endif
  SAFE_DELETE(g_prefsManager);
}

void GameApp::UpdateDifficultyFromPreferences()
{
  // This method is called to make sure that the difficulty setting
  // used to control the game play (g_app->m_difficultyLevel) is
  // consistent with the user preferences.

  // Preferences value is 1-based, m_difficultyLevel is 0-based.
  m_difficultyLevel = g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
  m_difficultyLevel = std::max(m_difficultyLevel, 0);
}

void GameApp::SetLanguage(const std::wstring& _language)
{
  //
  // Load localised fonts if they exist
  std::wstring lang = _language;
  if (auto it = std::ranges::find(g_supportedLanguages, _language); it == g_supportedLanguages.end())
  {
    lang = L"ENG";
  }
  else
    lang = *it;

  std::wstring fontFilename = L"Fonts\\SpeccyFont-" + lang + L".dds";
  g_gameFont.Startup(fontFilename);

  fontFilename = L"Fonts\\EditorFont-" + lang + L".dds";
  g_editorFont.Startup(fontFilename);
}

void GameApp::SetProfileName(const char* _profileName)
{
  m_userProfileName = _profileName;
}

const char* GameApp::GetProfileDirectory()
{
    return "";
}

const char* GameApp::GetPreferencesPath()
{
  // good leak #1
  static char* path = nullptr;

  if (path == nullptr)
  {
    const char* profileDir = GetProfileDirectory();
    path = NEW char[strlen(profileDir) + 32];
    sprintf(path, "%spreferences.txt", profileDir);
  }

  return path;
}

bool GameApp::LoadProfile()
{
  DebugTrace("Loading profile {}\n", m_userProfileName);

  if (m_userProfileName == "AccessAllAreas")
  {
    // Cheat username that opens all locations
    // aimed at beta testers who've completed the game already

    if (m_globalWorld)
    {
      delete m_globalWorld;
      m_globalWorld = nullptr;
    }

    m_globalWorld = NEW GlobalWorld();
    m_globalWorld->LoadGame("game_unlockall.txt");
    for (int i = 0; i < m_globalWorld->m_buildings.Size(); ++i)
    {
      GlobalBuilding* building = m_globalWorld->m_buildings[i];
      if (building && building->m_buildingType == BuildingType::TypeTrunkPort)
        building->m_online = true;
    }
    for (int i = 0; i < m_globalWorld->m_locations.Size(); ++i)
    {
      GlobalLocation* loc = m_globalWorld->m_locations[i];
      loc->m_available = true;
    }
  }
  else
  {
    if (m_globalWorld)
    {
      delete m_globalWorld;
      m_globalWorld = nullptr;
    }

    m_globalWorld = NEW GlobalWorld();
    m_globalWorld->LoadGame(m_gameDataFile);
  }

  return true;
}

void GameApp::LoadCampaign()
{
  m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

  m_gameDataFile = "game.txt";
  LoadProfile();
  m_gameMode = GameModeCampaign;
  m_requestedLocationId = -1;
  g_prefsManager->SetInt("RenderSpecialLighting", 0);
  g_prefsManager->SetInt("CurrentGameMode", 1);
  g_prefsManager->Save();
}
