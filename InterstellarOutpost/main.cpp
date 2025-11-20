#include "pch.h"
#include "main.h"
#include "Canvas.h"
#include "GameApp.h"
#include "LegacyVector3.h"
#include "StartSequence.h"
#include "camera.h"
#include "NetworkClient.h"
#include "explosion.h"
#include "file_paths.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "input.h"
#include "inputdriver_alias.h"
#include "inputdriver_chord.h"
#include "inputdriver_conjoin.h"
#include "inputdriver_idle.h"
#include "inputdriver_invert.h"
#include "inputdriver_prefs.h"
#include "inputdriver_value.h"
#include "inputdriver_win32.h"
#include "inputdriver_xinput.h"
#include "landscape.h"
#include "location.h"
#include "location_input.h"
#include "mainmenus.h"
#include "ParticleSystem.h"
#include "profiler.h"
#include "Renderer.h"
#include "resource.h"
#include "Script.h"
#include "Server.h"
#include "ServerToClientLetter.h"
#include "sound_library_2d.h"
#include "sound_library_3d_dsound.h"
#include "soundsystem.h"
#include "targetcursor.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "text_stream_readers.h"
#include "user_input.h"
#include "window_manager.h"

#define TARGET_FRAME_RATE_INCREMENT 0.25f

double g_startTime = DBL_MAX;
double g_gameTime = 0.0;
float g_advanceTime;
double g_lastServerAdvance;
float g_predictionTime;
float g_targetFrameRate = 20.0f;
int g_lastProcessedSequenceId = -1;
int g_sliceNum;							// Most recently advanced slice

void UpdateAdvanceTime()
{
  double realTime = GetHighResTime();
  g_advanceTime = static_cast<float>(realTime - g_gameTime);
  g_advanceTime = std::min(g_advanceTime, 0.25f);
  g_gameTime = realTime;

  g_predictionTime = static_cast<float>(realTime - g_lastServerAdvance) - 0.07f;
}

int GetNumSlicesToAdvance()
{
  float timeSinceStartOfAdvance = g_gameTime - g_lastServerAdvance;

  int numSlicesToAdvance = timeSinceStartOfAdvance * 100;
  if (g_sliceNum != -1)
    numSlicesToAdvance -= g_sliceNum;
  if (g_sliceNum == -1)
    numSlicesToAdvance -= 10;

  numSlicesToAdvance = std::max(numSlicesToAdvance, 0);
  numSlicesToAdvance = std::min(numSlicesToAdvance, 10);

  return numSlicesToAdvance;
}

bool ProcessServerLetters(ServerToClientLetter* letter)
{
  switch (letter->m_type)
  {
    case ServerToClientLetter::HelloClient:
      if (letter->m_ip == g_app->m_networkClient->GetOurIP_Int())
        DebugTrace("CLIENT : Received HelloClient from Server\n");
      return true;

    case ServerToClientLetter::GoodbyeClient:
      return true;

    case ServerToClientLetter::TeamAssign:

      if (letter->m_ip == g_app->m_networkClient->GetOurIP_Int())
        g_app->m_location->InitialiseTeam(letter->m_teamId, letter->m_teamType);
      else
        g_app->m_location->InitialiseTeam(letter->m_teamId, Team::TeamTypeRemotePlayer);
      return true;

    default:
      return false;
  }
}

bool WindowsOnScreen() { return Canvas::EclGetWindows()->Size() > 0; }

void RemoveAllWindows()
{
  LList<GuiWindow*>* windows = Canvas::EclGetWindows();
  while (windows->Size() > 0)
  {
    GuiWindow* w = windows->GetData(0);
    Canvas::EclRemoveWindow(w->m_name);
  }
}

unsigned char GenerateSyncValue() { return 255 * syncfrand(); }

bool LocationGameLoop()
{
  double nextServerAdvanceTime = GetHighResTime();
  double nextIAmAliveMessage = GetHighResTime();

  TeamControls teamControls;

  g_sliceNum = -1;

  g_app->m_renderer->StartFadeIn(0.6f);
  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "EnterLocation", SoundSourceBlueprint::TypeAmbience);

  //
  // Main loop

  bool fadingOut = false;
  while (!g_app->m_requestQuit)
  {
    if (!fadingOut)
    {
      if (g_app->m_requestedLocationId != g_app->m_locationId)
      {
        g_app->m_renderer->StartFadeOut();
        fadingOut = true;
      }
    }
    else
    {
      if (g_app->m_renderer->IsFadeComplete())
        break;
    }

    g_inputManager->PollForEvents();
    if (g_inputManager->controlEvent(ControlMenuEscape) && g_app->m_renderer->IsFadeComplete())
    {
      if (!g_app->m_script || !g_app->m_script->IsRunningScript())
      {
        if (WindowsOnScreen())
          RemoveAllWindows();
        else if (g_app->m_taskManagerInterface->m_visible)
          g_app->m_taskManagerInterface->m_visible = false;
        else
        {
          Canvas::EclRegisterWindow(NEW LocationWindow());
        }
      }
      g_app->m_userInput->Advance();
    }

    if (g_app->m_requestQuit)
      continue;

    //
    // Get the time
    double timeNow = GetHighResTime();

    //
    // Advance the server
    if (timeNow > nextServerAdvanceTime)
    {
      Server::Advance();
      nextServerAdvanceTime += SERVER_ADVANCE_PERIOD;
      if (timeNow > nextServerAdvanceTime)
        nextServerAdvanceTime = timeNow + SERVER_ADVANCE_PERIOD;
    }

    teamControls.Advance();

    START_PROFILE(g_app->m_profiler, "Client Main Loop");

    //
    // Send Client input to Server
    if (timeNow > nextIAmAliveMessage)
    {
      // Read the current teamControls from the inputManager

      bool entityUnderMouse = false;

      if (Team* team = g_app->m_location->GetMyTeam())
      {
        bool checkMouse = false;
        if (teamControls.m_unitMove)
          checkMouse = true;

        bool orderGiven = false;
        if (g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD && teamControls.m_primaryFireTarget)
          orderGiven = true;

        if (team->GetMyEntity() && team->GetMyEntity()->m_entityType == EntityType::TypeOfficer && orderGiven)
          checkMouse = true;

        if (checkMouse)
        {
          // We don't actually want to pass any left-clicks to the network system
          // If the user has left-clicked on another of his entities, because that
          // entity is about to be selected.  We don't want our original entity
          // walking up to him.
          WorldObjectId idUnderMouse;
          bool objectUnderMouse = g_app->m_locationInput->GetObjectUnderMouse(idUnderMouse, g_app->m_globalWorld->m_myTeamId);

          bool isCurrentEntity = (objectUnderMouse && idUnderMouse.GetUnitId() == -1 && 
            idUnderMouse.GetIndex() == team->m_currentEntityId);
          bool isCurrentUnit = (objectUnderMouse && idUnderMouse.GetUnitId() != -1 && 
            idUnderMouse.GetUnitId() == team->m_currentUnitId);

          entityUnderMouse = (objectUnderMouse && idUnderMouse.GetUnitId() != UNIT_BUILDINGS && !isCurrentEntity && !
            isCurrentUnit);

          if (idUnderMouse.GetUnitId() == UNIT_BUILDINGS)
          {
            // Focus the mouse on a Radar Dish if one exists under the mouse
            Building* building = g_app->m_location->GetBuilding(idUnderMouse.GetUniqueId());
            if (building && building->m_buildingType == BuildingType::TypeRadarDish)
              teamControls.m_mousePos = building->m_pos;
          }
        }
      }

      if (g_app->m_taskManagerInterface->m_visible || Canvas::EclGetWindows()->Size() != 0 || entityUnderMouse)
        teamControls.ClearFlags();

      g_app->m_networkClient->SendIAmAlive(g_app->m_globalWorld->m_myTeamId, teamControls);

      nextIAmAliveMessage += IAMALIVE_PERIOD;
      if (timeNow > nextIAmAliveMessage)
        nextIAmAliveMessage = timeNow + IAMALIVE_PERIOD;

      teamControls.Clear();
    }

    g_app->m_networkClient->Advance();

    int slicesToAdvance = GetNumSlicesToAdvance();

    END_PROFILE(g_app->m_profiler, "Client Main Loop");

    // Do our heavy weight physics
    for (int i = 0; i < slicesToAdvance; ++i)
    {
      if (g_sliceNum == -1)
      {
        // Read latest update from Server
        ServerToClientLetter* letter = g_app->m_networkClient->GetNextLetter();

        if (letter)
        {
          DEBUG_ASSERT(letter->GetSequenceId() == g_lastProcessedSequenceId + 1);

          bool handled = ProcessServerLetters(letter);
          if (handled == false)
            g_app->m_networkClient->ProcessServerUpdates(letter);

          g_sliceNum = 0;
          g_lastServerAdvance = static_cast<float>(letter->GetSequenceId()) * SERVER_ADVANCE_PERIOD + g_startTime;
          g_lastProcessedSequenceId = letter->GetSequenceId();
          delete letter;

          unsigned char sync = GenerateSyncValue();
          g_app->m_networkClient->SendSyncronisation(g_lastProcessedSequenceId, sync);
        }
      }

      if (g_sliceNum != -1)
      {
        g_app->m_location->Advance(g_sliceNum);
        g_app->m_particleSystem->Advance(g_sliceNum);

        if (g_sliceNum < NUM_SLICES_PER_FRAME - 1)
          g_sliceNum++;
        else
          g_sliceNum = -1;
      }
    }

    // Render
    UpdateAdvanceTime();
#ifdef PROFILER_ENABLED
    g_app->m_profiler->Advance();
#endif

    g_app->m_userInput->Advance();

    // The following are candidates for running in parallel
    // using something like OpenMP
    g_soundLibrary2d->TopupBuffer();
    g_app->m_camera->Advance();
    g_app->m_locationInput->Advance();
    g_app->m_taskManager->Advance();
    g_app->m_taskManagerInterface->Advance();
    g_app->m_script->Advance();
    g_explosionManager.Advance();
    g_app->m_soundSystem->Advance();

    // DELETEME: for debug purposes only
    g_app->m_globalWorld->EvaluateEvents();

    g_app->m_renderer->Render();

    if (g_app->m_renderer->m_fps < 15)
      g_app->m_soundSystem->Advance();
  }

  g_app->m_soundSystem->StopAllSounds(WorldObjectId(), "Ambience EnterLocation");
  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "ExitLocation", SoundSourceBlueprint::TypeAmbience);

  g_explosionManager.Reset();

  if (!g_app->m_globalWorld->GetLocationName(g_app->m_locationId).empty())
    g_app->m_globalWorld->TransferSpirits(g_app->m_locationId);

  g_app->m_networkClient->ClientLeave();
  g_app->m_location->Empty();
  g_app->m_particleSystem->Empty();

  delete g_app->m_location;
  g_app->m_location = nullptr;
  g_app->m_locationId = -1;

  delete g_app->m_locationInput;
  g_app->m_locationInput = nullptr;

  g_app->m_taskManager->StopAllTasks();

  g_app->m_globalWorld->m_myTeamId = 255;
  g_app->m_globalWorld->EvaluateEvents();

  return g_app->m_requestQuit;
}

bool GlobalWorldGameLoop()
{
  g_app->m_renderer->StartFadeIn(0.25f);

  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "EnterGlobalWorld", SoundSourceBlueprint::TypeAmbience);

  while (g_app->m_requestedLocationId == -1 && !g_app->m_requestQuit)
  {
    if (g_app->m_atMainMenu)
      break;

    g_inputManager->PollForEvents();

    if (g_inputManager->controlEvent(ControlMenuEscape) && g_app->m_renderer->IsFadeComplete())
    {
      if (WindowsOnScreen())
        RemoveAllWindows();
      else
      {
        Canvas::EclRegisterWindow(NEW MainMenuWindow());
      }
      g_app->m_userInput->Advance();
    }

    if (g_app->m_requestQuit)
      continue;

    // Get the time
    UpdateAdvanceTime();

    g_app->m_script->Advance();
    g_app->m_globalWorld->Advance();
    g_app->m_userInput->Advance();
    g_app->m_camera->Advance();
    g_app->m_soundSystem->Advance();

#ifdef PROFILER_ENABLED
    g_app->m_profiler->Advance();
#endif // PROFILER_ENABLED

    g_app->m_globalWorld->EvaluateEvents();

    g_app->m_renderer->Render();
  }

  g_app->m_soundSystem->StopAllSounds(WorldObjectId(), "Ambience EnterGlobalWorld");
  return g_app->m_requestQuit;
}

void InitialiseInputManager()
{
  g_inputManager = NEW InputManager;
  g_inputManager->addDriver(NEW ConjoinInputDriver());
  g_inputManager->addDriver(NEW ChordInputDriver());
  g_inputManager->addDriver(NEW InvertInputDriver());
  g_inputManager->addDriver(NEW IdleInputDriver());
  g_inputManager->addDriver(NEW W32InputDriver());
  g_inputManager->addDriver(NEW PrefsInputDriver());
  g_inputManager->addDriver(NEW ValueInputDriver());
  g_inputManager->addDriver(NEW AliasInputDriver());
  {
    // Read Darwinia default input preferences file
    TextReader* inputPrefsReader = Resource::GetTextReader(InputPrefs::GetSystemPrefsPath());
    if (inputPrefsReader)
    {
      ASSERT_TEXT(inputPrefsReader->IsOpen(), "Couldn't open input preferences file: %s\n", InputPrefs::GetSystemPrefsPath());
      g_inputManager->parseInputPrefs(*inputPrefsReader);
      delete inputPrefsReader;
    }

    // Override again with user specified bindings
    TextReader* userInputPrefsReader = NEW TextFileReader(InputPrefs::GetUserPrefsPath());
    if (userInputPrefsReader)
    {
      if (userInputPrefsReader->IsOpen())
        g_inputManager->parseInputPrefs(*userInputPrefsReader, true);
      delete userInputPrefsReader;
    }
  }
}

void Initialize()
{
  //
  // Initialize all our basic objects

  InitialiseHighResTime();

  InitialiseInputManager();

  g_target = NEW TargetCursor();
  EntityBlueprint::Initialize();

  g_app->m_renderer->SetOpenGLState();
}

void Finalize()
{
  g_soundLibrary2d->Stop();
  delete g_soundLibrary3d;
  g_soundLibrary3d = nullptr;
  delete g_soundLibrary2d;
  g_soundLibrary2d = nullptr;

  delete g_windowManager;
}

void RunBootLoaders()
{
  g_app->m_startSequence = NEW StartSequence();
  while (true)
  {
    UpdateAdvanceTime();
    if (g_app->m_startSequence->Advance())
      break;
  }

  delete g_app->m_startSequence;
  g_app->m_startSequence = nullptr;

  g_app->m_camera->SetTarget(LegacyVector3(1000, 500, 1000), LegacyVector3(0, -0.5f, -1));
  g_app->m_camera->CutToTarget();

  g_inputManager->Advance();          // clears g_keyDeltas[KEY_ESC]
  g_inputManager->Advance();
}

bool EnterLocation()
{
  g_app->m_networkClient->ClientJoin();

  g_app->m_location = NEW Location();
  g_app->m_locationInput = NEW LocationInput();
  g_app->m_location->Init(g_app->m_requestedMission, g_app->m_requestedMap);
  g_app->m_locationId = g_app->m_requestedLocationId;

  g_app->m_camera->UpdateEntityTrackingMode();

  g_app->m_networkClient->RequestTeam(Team::TeamTypeCPU, -1);
  g_app->m_networkClient->RequestTeam(Team::TeamTypeCPU, -1);
  g_app->m_networkClient->RequestTeam(Team::TeamTypeLocalPlayer, -1);

  constexpr float borderSize = 200.0f;
  float minX = -borderSize;
  float maxX = g_app->m_location->m_landscape.GetWorldSizeX() + borderSize;
  g_app->m_camera->SetBounds(minX, maxX, minX, maxX);
  g_app->m_camera->SetTarget(LegacyVector3(maxX, 1000, maxX), LegacyVector3(-1, -0.7, -1)); // Incase start doesn't exist
  g_app->m_camera->SetTarget("start");
  g_app->m_camera->CutToTarget();
  g_app->m_camera->RequestMode(Camera::ModeFreeMovement);

  return LocationGameLoop();
}

bool EnterGlobalWorld()
{
  // Put the camera in a sensible place
  g_app->m_camera->RequestMode(Camera::ModeSphereWorld);
  g_app->m_camera->SetHeight(50.0f);

  return GlobalWorldGameLoop();
}

// Main Function
void AppMain()
{
  bool quit = false;

  Server::Startup();

  Initialize();
  RunBootLoaders();

  g_app->LoadCampaign();

  while (!quit)
  {
    if (g_app->m_requestedLocationId != -1)
      quit = EnterLocation();
    else 
      quit = EnterGlobalWorld();

    g_inputManager->Advance();
  }

  Finalize();

  Server::Shutdown();
}
