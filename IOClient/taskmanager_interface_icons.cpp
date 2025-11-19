#include "pch.h"
#include "taskmanager_interface_icons.h"
#include "Canvas.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "LegacyVector2.h"
#include "camera.h"
#include "NetworkClient.h"
#include "engineer.h"
#include "gamecursor.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "input.h"
#include "insertion_squad.h"
#include "level_file.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "ParticleSystem.h"
#include "profiler.h"
#include "Renderer.h"
#include "researchitem.h"
#include "resource.h"
#include "Script.h"
#include "soundsystem.h"
#include "targetcursor.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"

TaskManagerInterfaceIcons::TaskManagerInterfaceIcons()
  : m_screenId(ScreenTaskManager),
  m_screenY(0.0f),
  m_desiredScreenY(0.0f),
  m_screenW(800),
  m_screenH(600),
  m_chatLogY(0.0f),
  m_currentScreenZone(-1),
  m_currentMouseScreenZone(-1),
  m_currentScrollZone(1),
  m_screenZoneTimer(0.0f),
  m_taskManagerDownTime(0.0),
  m_currentQuickUnit(-1),
  m_quickUnitDirection(0)
{
  m_viewingDefaultObjective = false;
  m_messageTimer = 0.0f;
  m_currentTaskType = -1;
  m_currentMessageType = -1;
  m_highlightedTaskId = -1;
  SetVisible(false);

  Resource::GetTexture("textures\\interface_grey.bmp", true, false);
  Resource::GetTexture("textures\\interface_red.bmp", true, false);
  Resource::GetTexture("icons\\icon_shadow.bmp", true, false);

  //
  // Create keyboard shortcuts

  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("NewTask", GlobalResearch::TypeSquad, ControlIconsTaskManagerNewSquad));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("NewTask", GlobalResearch::TypeEngineer, ControlIconsTaskManagerNewEngineer));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("NewTask", GlobalResearch::TypeOfficer, ControlIconsTaskManagerNewOfficer));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("NewTask", GlobalResearch::TypeArmour, ControlIconsTaskManagerNewArmour));

  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectWeapon", GlobalResearch::TypeGrenade,
    ControlIconsTaskManagerSelectGrenade));
  m_keyboardShortcuts.
    PutData(NEW KeyboardShortcut("SelectWeapon", GlobalResearch::TypeRocket, ControlIconsTaskManagerSelectRocket));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectWeapon", GlobalResearch::TypeAirStrike,
    ControlIconsTaskManagerSelectAirStrike));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectWeapon", GlobalResearch::TypeController,
    ControlIconsTaskManagerSelectController));

  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 0, ControlTaskManagerSelectTask1));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 1, ControlTaskManagerSelectTask2));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 2, ControlTaskManagerSelectTask3));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 3, ControlTaskManagerSelectTask4));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 4, ControlTaskManagerSelectTask5));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 5, ControlTaskManagerSelectTask6));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 6, ControlTaskManagerSelectTask7));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 7, ControlTaskManagerSelectTask8));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("SelectTask", 8, ControlTaskManagerSelectTask9));

  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("DeleteTask", -1, ControlTaskManagerEndTask));

  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("ScreenUp", -1, ControlUnitCycleLeft));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("ScreenDown", -1, ControlUnitCycleRight));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("ScreenUp", -1, ControlCameraForwards));
  m_keyboardShortcuts.PutData(NEW KeyboardShortcut("ScreenDown", -1, ControlCameraBackwards));
}

void TaskManagerInterfaceIcons::AdvanceTerminate()
{
  if (g_inputManager->controlEvent(ControlIconsTaskManagerEndTask))
  {
    g_app->m_taskManager->TerminateTask(g_app->m_taskManager->m_currentTaskId);
    g_app->m_taskManager->m_currentTaskId = -1;
  }
}

void TaskManagerInterfaceIcons::HideTaskManager()
{
  //
  // Put the mouse to the middle of the screen
  float midX = ClientEngine::OutputSize().Width / 2.0f;
  float midY = ClientEngine::OutputSize().Height / 2.0f;
  g_target->SetMousePos(midX, midY);
  g_app->m_camera->Advance();

  if (g_app->m_taskManager->m_tasks.Size() > 0)
    g_app->m_taskManager->SelectTask(g_app->m_taskManager->m_currentTaskId);

  m_screenY = 0.0f;
  m_desiredScreenY = 0.0f;
  m_screenId = ScreenTaskManager;
  SetVisible(false);

  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Hide", SoundSourceBlueprint::TypeInterface);
}

void TaskManagerInterfaceIcons::Advance()
{
  if (m_lockTaskManager)
    return;

  AdvanceScrolling();
  AdvanceTab();

  if (m_viewingDefaultObjective)
  {
    // We were running a default objective description (trunk port, research item)
    // So shut it down now
    g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
    m_viewingDefaultObjective = false;
  }

  bool inCutscene = false;
  if (g_app->m_script->IsRunningScript() && g_app->m_script->m_permitEscape)
    inCutscene = true;
  if (g_app->m_camera->IsInMode(Camera::ModeBuildingFocus))
    inCutscene = true;

  if (inCutscene)
  {
    if (m_visible)
      HideTaskManager();
    return;
  }

  if (!m_visible && (g_inputManager->controlEvent(ControlIconsTaskManagerDisplay) || g_inputManager->controlEvent(
    ControlIconsTaskManagerDisplayDown)))
  {
    // Tab key just pressed
    // Pop up the task manager
    m_screenY = 0.0f;
    m_desiredScreenY = 0.0f;
    m_screenId = ScreenTaskManager;
    m_currentScrollZone = 1;

    SetVisible();

    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);

    if (g_inputManager->controlEvent(ControlIconsTaskManagerDisplayDown))
    {
      // Controller activated the task manager, record the time
      m_taskManagerDownTime = GetHighResTime();
    }
  }
  else if (m_visible && g_inputManager->controlEvent(ControlIconsTaskManagerHide))
  {
    // Tab key pressed while visible
    // So remove the task manager
    HideTaskManager();
  }

  if (m_visible)
    AdvanceScreenEdges();
  AdvanceScreenZones();
  AdvanceKeyboardShortcuts();
  AdvanceTerminate();
  AdvanceQuickUnit();

  if (g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnit))
  {
    if (!m_quickUnitVisible)
      CreateQuickUnitInterface();
    else
      DestroyQuickUnitInterface();
  }
}

void TaskManagerInterfaceIcons::AdvanceScrolling()
{
  if (m_screenY != m_desiredScreenY)
  {
    float diff = g_advanceTime * 10;
    m_screenY = m_screenY * (1.0f - diff) + (m_desiredScreenY * diff);
    if (fabs(m_desiredScreenY - m_screenY) < 0.001f)
      m_screenY = m_desiredScreenY;
  }
}

void TaskManagerInterfaceIcons::AdvanceScreenEdges()
{
  bool scrollPermitted = fabs(m_desiredScreenY - m_screenY) == 0;

  int screenBorder = 10;

  bool scrollRequested = false;
  switch (m_screenId)
  {
  case ScreenTaskManager:
    if (scrollPermitted)
    {
      if (g_target->Y() < screenBorder)
      {
        m_screenId = ScreenResearch;
        m_desiredScreenY = -1.0f;
        scrollRequested = true;
      }
      if (g_target->Y() > ClientEngine::OutputSize().Height - screenBorder)
      {
        m_screenId = ScreenObjectives;
        m_desiredScreenY = 1.0f;
        scrollRequested = true;
      }
    }
    break;

  case ScreenObjectives:
    if (scrollPermitted)
    {
      if (g_target->Y() < screenBorder)
      {
        m_screenId = ScreenTaskManager;
        m_desiredScreenY = 0.0f;
        scrollRequested = true;
      }
    }
    break;

  case ScreenResearch:
    if (scrollPermitted)
    {
      if (g_target->Y() > ClientEngine::OutputSize().Height - screenBorder)
      {
        m_screenId = ScreenTaskManager;
        m_desiredScreenY = 0.0f;
        scrollRequested = true;
      }
    }
    break;
  }

  if (scrollRequested)
    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
}

void TaskManagerInterfaceIcons::SetupRenderMatrices(int _screenId)
{
  /*
   *	This is designed to make rendering of the individual screens much easier.
   *  It maps the screen co-ordinates so that each renderer can simply render
   *  to a fixed size window, without having to worry about scrolling, screen resolution etc
   *  The window will be 800x600 on a normal screen
   *  And 960x600 on a widescreen screen
   */

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  float screenRatio = static_cast<float>(ClientEngine::OutputSize().Width) / static_cast<float>(ClientEngine::OutputSize().Height);
  m_screenH = 600.0f;
  m_screenW = m_screenH * screenRatio;

  float left = 0;
  float top = 0;

  switch (_screenId)
  {
  case ScreenOverlay:
    break;

  case ScreenTaskManager:
    top += m_screenY * m_screenH;
    break;

  case ScreenObjectives:
    top += (m_screenY - 1.0f) * m_screenH;
    break;

  case ScreenResearch:
    top += (m_screenY + 1.0f) * m_screenH;
    break;
  }

  float right = left + m_screenW;
  float bottom = top + m_screenH;

  gluOrtho2D(left, right, bottom, top);
  glMatrixMode(GL_MODELVIEW);
}

void TaskManagerInterfaceIcons::ConvertMousePosition(float& _x, float& _y)
{
  float fractionX = _x / static_cast<float>(ClientEngine::OutputSize().Width);
  float fractionY = _y / static_cast<float>(ClientEngine::OutputSize().Height);

  _x = m_screenW * fractionX;
  _y = m_screenH * fractionY;
}

void TaskManagerInterfaceIcons::RestoreRenderMatrices() { g_app->m_renderer->SetupMatricesFor2D(); }

void TaskManagerInterfaceIcons::Render()
{
  if (!g_app->m_location || Canvas::EclGetWindows()->Size())
    return;

  START_PROFILE(g_app->m_profiler, "Render Taskman");

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  if (g_app->m_locationId != -1)
    RenderTargetAreas();

  g_gameFont.BeginText2D();
  SetupRenderMatrices(ScreenOverlay);

  RenderMessages();

  if (m_visible)
  {
    RenderTitleBar();

    if (m_screenY > -1.0f && m_screenY < 1.0f)
      RenderTaskManager();
    if (m_screenY < 0.0f)
      RenderResearch();
    if (m_screenY > 0.0f)
      RenderObjectives();

    SetupRenderMatrices(ScreenOverlay);

    //
    // Render silver bar

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_divider.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(0, m_screenH - 26);
    glTexCoord2i(100, 0);
    glVertex2i(m_screenW, m_screenH - 26);
    glTexCoord2i(100, 1);
    glVertex2i(m_screenW, m_screenH);
    glTexCoord2i(0, 1);
    glVertex2i(0, m_screenH);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    RenderTooltip();
    RenderScreenZones();
  }

  RestoreRenderMatrices();

  SetupRenderMatrices(ScreenOverlay);

  if (!m_visible)
    RenderRunningTasks();
  RestoreRenderMatrices();

  //
  // Render a mouse cursor if we are visible
  // The normal one is obscured by the task manager

  if (m_visible)
  {
    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    g_app->m_gameCursor->RenderStandardCursor(mouseX, mouseY);
  }

  g_gameFont.EndText2D();

  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  END_PROFILE(g_app->m_profiler, "Render Taskman");
}

void TaskManagerInterfaceIcons::AdvanceScreenZones()
{
  //
  // Copy the newly generated screenzone list over
  // Remove the old set of screen zones

  m_screenZones.EmptyAndDelete();
  while (m_newScreenZones.Size() > 0)
  {
    ScreenZone* zone = m_newScreenZones[0];
    m_screenZones.PutData(zone);
    m_newScreenZones.RemoveData(0);
    if (m_currentScreenZone == -1 && zone->m_scrollZone == m_currentScrollZone)
      m_currentScreenZone = m_screenZones.Size() - 1;
  }

  //
  // Where is the mouse right now

  bool found = false;

  if (m_visible)
  {
    for (int i = 0; i < m_screenZones.Size(); ++i)
    {
      ScreenZone* zone = m_screenZones[i];
      if (ScreenZoneHighlighted(zone))
      {
        if (m_currentScreenZone != i && m_currentMouseScreenZone != i)
        {
          m_currentMouseScreenZone = i;
          m_currentScreenZone = i;
          m_screenZoneTimer = GetHighResTime();
          g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
        }
        found = true;
      }
    }

    if (g_inputManager->controlEvent(ControlMenuDown) && m_currentScrollZone != 3)
    {
      int numZones = m_screenZones.Size();
      int zonesRemaining = numZones;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
      while (zonesRemaining)
      {
        m_currentScreenZone = (m_currentScreenZone + 1) % numZones;
        zonesRemaining--;

        if (m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone ==
          m_currentScrollZone)
          break;
      }
    }

    if (g_inputManager->controlEvent(ControlMenuUp) && m_currentScrollZone != 3)
    {
      int numZones = m_screenZones.Size();
      int zonesRemaining = numZones;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
      while (zonesRemaining)
      {
        m_currentScreenZone = (m_currentScreenZone + numZones - 1) % numZones;
        zonesRemaining--;

        if (m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone ==
          m_currentScrollZone)
          break;
      }
    }

    bool changeScrollZone = false;
    if (m_screenId == ScreenTaskManager)
    {
      if (g_inputManager->controlEvent(ControlMenuLeft))
      {
        if (m_currentScrollZone == 1)
        {
          m_currentScrollZone = 2;
          changeScrollZone = true;
        }
        else if (m_currentScrollZone == 3)
        {
          while (true)
          {
            m_currentScreenZone--;
            if (m_currentScreenZone < 0)
            {
              m_currentScrollZone = 2;
              changeScrollZone = true;
              break;
            }

            if ((m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone ==
              m_currentScrollZone) || m_screenZones.Size() == 0)
              break;
          }
        }
      }

      if (g_inputManager->controlEvent(ControlMenuRight))
      {
        if (m_currentScrollZone == 2)
        {
          if (m_screenZones[m_currentScreenZone]->m_subZones)
            m_currentScrollZone = 3;
          else
            m_currentScrollZone = 1;
          changeScrollZone = true;
        }
        else if (m_currentScrollZone == 3)
        {
          while (true)
          {
            m_currentScreenZone++;
            if (m_currentScreenZone >= m_screenZones.Size())
            {
              m_currentScrollZone = 1;
              changeScrollZone = true;
              break;
            }

            if ((m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone ==
              m_currentScrollZone) || m_screenZones.Size() == 0)
              break;
          }
        }
      }
    }

    if (changeScrollZone)
    {
      m_currentScreenZone = -1;
      for (int i = 0; i < m_screenZones.Size(); ++i)
      {
        if (m_screenZones[i]->m_scrollZone == m_currentScrollZone)
        {
          m_currentMouseScreenZone = i;
          m_currentScreenZone = i;
          m_screenZoneTimer = GetHighResTime();
          g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
          break;
        }
      }
      if (m_currentScreenZone == -1)
        m_currentScrollZone = (m_currentScreenZone == 1) ? 2 : 1;
    }
  }

  //
  // Special case - the player is highlighting a unit/entity in the real world
  // We want to highlight the button that corrisponds to that task ID
  // (if it exists)

  bool highlightOnly = false;

  if (!m_visible)
  {
    m_currentScreenZone = -1;
    WorldObjectId id;
    bool somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse(id, g_app->m_globalWorld->m_myTeamId);
    if (somethingHighlighted)
    {
      int taskIndex = -1;
      for (int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i)
      {
        if (g_app->m_taskManager->m_tasks[i]->m_objId == id)
        {
          taskIndex = i;
          break;
        }
      }

      if (taskIndex != -1)
      {
        for (int i = 0; i < m_screenZones.Size(); ++i)
        {
          ScreenZone* zone = m_screenZones[i];
          if (zone->m_name == "SelectTask" && zone->m_data == taskIndex)
          {
            if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD)
            {
              //m_currentScreenZone = i;
            }
            found = true;
            highlightOnly = true;
          }
        }
      }
    }
  }

  //if( !found ) m_currentScreenZone = -1;

  //
  // Are we highlighting a task?

  m_highlightedTaskId = -1;
  if (m_currentScreenZone != -1)
  {
    ScreenZone* currentZone = m_screenZones[m_currentScreenZone];
    if (currentZone)
    {
      if (currentZone->m_name == "SelectTask" && g_app->m_taskManager->m_tasks.ValidIndex(currentZone->m_data))
      {
        Task* task = g_app->m_taskManager->m_tasks[currentZone->m_data];
        m_highlightedTaskId = task->m_id;
      }
    }
  }

  //
  // Handle mouse clickage

  if (m_currentScreenZone != -1 && (g_inputManager->controlEvent(ControlActivateTMButton) || ButtonHeldAndReleased()) && !
    highlightOnly)
  {
    ScreenZone* currentZone = m_screenZones[m_currentScreenZone];
    if (currentZone)
      RunScreenZone(currentZone->m_name, currentZone->m_data);
  }
}

bool TaskManagerInterfaceIcons::ButtonHeld() { return GetHighResTime() - m_taskManagerDownTime > 0.5; }

bool TaskManagerInterfaceIcons::ButtonHeldAndReleased()
{
  return ButtonHeld() && g_inputManager->controlEvent(ControlIconsTaskManagerDisplayUp);
}

void TaskManagerInterfaceIcons::AdvanceKeyboardShortcuts()
{
  for (int i = 0; i < m_keyboardShortcuts.Size(); ++i)
  {
    KeyboardShortcut* shortcut = m_keyboardShortcuts[i];
    if ((*shortcut)())
      RunScreenZone(shortcut->name(), shortcut->data());
  }
}

void TaskManagerInterfaceIcons::RunScreenZone(std::string_view _name, int _data)
{
  //
  // New task buttons

  if (_name == "NewTask")
  {
    //
    // Prevent creation of new tasks if we're placing one

    Task* task = g_app->m_taskManager->GetCurrentTask();
    if (task && task->m_state == Task::StateStarted && task->m_type != GlobalResearch::TypeOfficer)
    {
      SetCurrentMessage(MessageFailure, -1, 2.5f);
      return;
    }

    if (g_app->m_globalWorld->m_research->HasResearch(_data))
    {
      g_app->m_networkClient->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, _data);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);
      if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
        HideTaskManager();
    }
    else
      SetCurrentMessage(MessageFailure, -1, 2.5f);
  }

  //
  // Select task buttons

  if (_name == "SelectTask")
  {
    if (g_app->m_taskManager->m_tasks.ValidIndex(_data))
    {
      Task* nextTask = g_app->m_taskManager->m_tasks[_data];
      g_app->m_taskManager->m_currentTaskId = nextTask->m_id;
      g_app->m_taskManager->SelectTask(g_app->m_taskManager->m_currentTaskId);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "SelectTask", SoundSourceBlueprint::TypeInterface);
      if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
        HideTaskManager();
    }
  }

  //
  // Delete task buttons

  if (_name == "DeleteTask")
  {
    if (_data == -1)
      g_app->m_taskManager->TerminateTask(g_app->m_taskManager->m_currentTaskId);
    else
      g_app->m_taskManager->TerminateTask(_data);
    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "DeleteTask", SoundSourceBlueprint::TypeInterface);
  }

  //
  // Select weapon buttons

  if (_name == "SelectWeapon")
  {
    if (g_app->m_globalWorld->m_research->HasResearch(_data))
    {
      g_app->m_networkClient->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, _data);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);
      if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
        HideTaskManager();
    }
    else
      SetCurrentMessage(MessageFailure, -1, 2.5f);
  }

  //
  // Show screen

  if (m_visible)
  {
    if (_name == "ShowScreen")
    {
      m_screenId = _data;

      switch (m_screenId)
      {
      case ScreenOverlay:
      case ScreenTaskManager:
        m_desiredScreenY = 0.0f;
        break;
      case ScreenObjectives:
        m_desiredScreenY = 1.0f;
        break;
      case ScreenResearch:
        m_desiredScreenY = -1.0f;
        break;
      }

      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
    }

    if (_name == "ScreenUp")
    {
      switch (m_screenId)
      {
      case ScreenOverlay:
      case ScreenTaskManager:
        RunScreenZone("ShowScreen", ScreenResearch);
        break;
      case ScreenObjectives:
        RunScreenZone("ShowScreen", ScreenTaskManager);
        break;
      }
    }

    if (_name == "ScreenDown")
    {
      switch (m_screenId)
      {
      case ScreenOverlay:
      case ScreenTaskManager:
        RunScreenZone("ShowScreen", ScreenObjectives);
        break;
      case ScreenResearch:
        RunScreenZone("ShowScreen", ScreenTaskManager);
        break;
      }
    }
  }

  //
  // Research

  if (_name == "Research")
    g_app->m_globalWorld->m_research->SetCurrentResearch(_data);

  //
  // Objectives

  if (_name == "Objective")
  {
    bool primary = _data > 0;
    int objectiveId = _data;
    if (!primary)
      objectiveId *= -1;
    objectiveId -= 10;

    GlobalEventCondition* gec = nullptr;
    if (primary)
      gec = g_app->m_location->m_levelFile->m_primaryObjectives[objectiveId];
    else
      gec = g_app->m_location->m_levelFile->m_secondaryObjectives[objectiveId];
    DEBUG_ASSERT(gec);

    if (!gec->m_cutScene.empty())
      g_app->m_script->RunScript(gec->m_cutScene);
    else
      RunDefaultObjective(gec);
    HideTaskManager();
  }
}

bool TaskManagerInterfaceIcons::ScreenZoneHighlighted(const ScreenZone* _zone)
{
  float mouseX = g_target->X();
  float mouseY = g_target->Y();
  ConvertMousePosition(mouseX, mouseY);

  mouseY += m_screenY * m_screenH;

  return (mouseX >= _zone->m_x && mouseY >= _zone->m_y && mouseX <= _zone->m_x + _zone->m_w && mouseY <= _zone->m_y + _zone->m_h);
}

void TaskManagerInterfaceIcons::RenderScreenZones()
{
  if (m_screenZones.ValidIndex(m_currentScreenZone))
  {
    ScreenZone* zone = m_screenZones[m_currentScreenZone];

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float overSpill = zone->m_h * 0.1f;
    float hX = zone->m_x - overSpill;
    float hY = zone->m_y - overSpill;
    float hW = zone->m_w + overSpill * 2;
    float hH = zone->m_h + overSpill * 2;

    //hX -= m_screenY * m_screenW;
    hY -= m_screenY * m_screenH;

    glColor4f(0.05f, 0.05f, 0.5f, 0.3f);

    glBegin(GL_QUADS);
    glVertex2f(hX, hY);
    glVertex2f(hX + hW, hY);
    glVertex2f(hX + hW, hY + hH);
    glVertex2f(hX, hY + hH);
    glEnd();

    glColor4f(1.0f, 1.0f, 0.3f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\mouse_selection.bmp"));
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(hX, hY);
    glTexCoord2f(0.5f, 0.0f);
    glVertex2f(hX + hH / 2, hY);
    glTexCoord2f(0.5f, 1.0f);
    glVertex2f(hX + hH / 2, hY + hH);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(hX, hY + hH);

    glTexCoord2f(0.5f, 0.0f);
    glVertex2f(hX + hW - hH / 2, hY);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(hX + hW, hY);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(hX + hW, hY + hH);
    glTexCoord2f(0.5f, 1.0f);
    glVertex2f(hX + hW - hH / 2, hY + hH);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
  }
}

void TaskManagerInterfaceIcons::RenderTooltip()
{
  g_gameFont.SetRenderShadow(true);
  glColor4ub(255, 255, 150, 30);

  if (m_screenZones.ValidIndex(m_currentScreenZone))
  {
    ScreenZone* zone = m_screenZones[m_currentScreenZone];

    //
    // Render tooltip caption

    float timeRequired = zone->m_toolTip.size() / 50.0f;
    float timeSoFar = GetHighResTime() - m_screenZoneTimer;

    float fraction = (timeSoFar < timeRequired) ? timeSoFar / timeRequired : 1.0f;

    g_gameFont.DrawText2D(20, m_screenH - 12, 12, zone->m_toolTip.substr(0, zone->m_toolTip.size() * fraction));

    if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD)
    {
      //
      // Render keyboard shortcut

      KeyboardShortcut* selectedShortcut = nullptr;

      for (int i = 0; i < m_keyboardShortcuts.Size(); ++i)
      {
        KeyboardShortcut* shortcut = m_keyboardShortcuts[i];
        if (shortcut->name() == zone->m_name && shortcut->data() == zone->m_data)
        {
          selectedShortcut = shortcut;
          break;
        }

        if (shortcut->name() == zone->m_name && shortcut->name() == "DeleteTask")
        {
          // Special case : DeleteTask shortcut key
          selectedShortcut = shortcut;
          break;
        }
      }

      if (selectedShortcut)
      {
        char caption[256];

        sprintf(caption, "Keyboard shortcut : %s", selectedShortcut->noun().c_str());

        g_gameFont.DrawText2D(m_screenW - 250, m_screenH - 12, 12, caption);
        g_gameFont.DrawText2D(m_screenW - 250, m_screenH - 12, 12, caption);
      }
    }
  }

  g_gameFont.SetRenderShadow(false);
}

void TaskManagerInterfaceIcons::RenderMessages()
{
  if (m_currentMessageType != -1)
  {
    //
    // Are we done yet?

    if (GetHighResTime() >= m_messageTimer)
    {
      m_currentMessageType = -1;
      return;
    }

    //
    // Lookup message portion

    char currentMessageStringId[256];
    sprintf(currentMessageStringId, "taskmanager_msg%d", m_currentMessageType);
    auto message = Strings::Get(currentMessageStringId);

    //
    // Lookup task name

    std::string taskName;

    if (m_currentTaskType != -1)
      taskName = Task::GetTaskNameTranslated(m_currentTaskType);

    //
    // Build string

    char fullMessage[256];
    if (!taskName.empty())
    {
      if (m_currentMessageType == MessageResearchUpgrade)
      {
        int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel(m_currentTaskType);
        sprintf(fullMessage, "%s: %s v%d.0", message.c_str(), taskName.c_str(), researchLevel);
      }
      else
        sprintf(fullMessage, "%s: %s", message.c_str(), taskName.c_str());
    }
    else
      sprintf(fullMessage, "%s", message.c_str());

    //
    // Render string

    float timeRemaining = m_messageTimer - GetHighResTime();
    float alpha = timeRemaining * 0.5f;
    alpha = std::min(alpha, 1.0f);
    alpha = std::max(alpha, 0.0f);
    float size = 40.0f;
    if (timeRemaining < 2.0f)
    {
      size += sqrtf(2.0f - timeRemaining) * 15.0f;
    }

    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    g_gameFont.DrawText2DCenter(m_screenW / 2.0f, 370.0f, size, fullMessage);
  }
}

void TaskManagerInterfaceIcons::RenderTargetAreas()
{
  Task* task = g_app->m_taskManager->GetCurrentTask();

  if (task && task->m_type != GlobalResearch::TypeOfficer && task->m_state == Task::StateStarted)
  {
    LList<TaskTargetArea>* targetAreas = g_app->m_taskManager->GetTargetArea(task->m_id);

    for (int i = 0; i < targetAreas->Size(); ++i)
    {
      TaskTargetArea* tta = targetAreas->GetPointer(i);

      float angle = g_gameTime * 3.0f;
      LegacyVector3 dif(tta->m_radius * sinf(angle), 0.0f, tta->m_radius * cosf(angle));

      LegacyVector3 pos = tta->m_center + dif;
      pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 5.0f;
      g_app->m_particleSystem->CreateParticle(pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0f);

      pos = tta->m_center - dif;
      pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 5.0f;
      g_app->m_particleSystem->CreateParticle(pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0f);
    }

    delete targetAreas;
  }
}

void TaskManagerInterfaceIcons::RenderTaskManager()
{
  SetupRenderMatrices(ScreenTaskManager);

  glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  //
  // Render running tasks

  RenderRunningTasks();

  //
  // Render task creation menu

  RenderCreateTaskMenu();

  //
  // Render navigation arrows

  if (fabs(m_screenY) < 0.1f)
  {
    unsigned alpha = (fmodf(g_gameTime, 2.0f) < 1.0f ? 155 : 255);

    glColor4ub(199, 214, 220, alpha);

    glBegin(GL_TRIANGLES);
    glVertex2f(m_screenW - 40, 10);
    glVertex2f(m_screenW - 20, 30);
    glVertex2f(m_screenW - 60, 30);

    glVertex2f(m_screenW - 40, m_screenH - 30);
    glVertex2f(m_screenW - 20, m_screenH - 50);
    glVertex2f(m_screenW - 60, m_screenH - 50);
    glEnd();

    auto zoneLeft = NEW ScreenZone("ScreenUp", Strings::Get("newcontrols_showresearch"), m_screenW - 60, 10, 40, 20, -1);
    auto zoneRight = NEW ScreenZone("ScreenDown", Strings::Get("newcontrols_showobjectives"), m_screenW - 60, m_screenH - 50, 40,
      20, -1);

    m_newScreenZones.PutData(zoneLeft);
    m_newScreenZones.PutData(zoneRight);
  }
}

void TaskManagerInterfaceIcons::RenderCreateTaskMenu()
{
  int numRunnableTasks = 4;
  int runnableTaskType[] = {
    GlobalResearch::TypeSquad,
    GlobalResearch::TypeEngineer,
    GlobalResearch::TypeOfficer,
    GlobalResearch::TypeArmour
  };

  int numAvailable = 0;
  for (int i = 0; i < numRunnableTasks; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      numAvailable++;
  }

  float w = 200;
  float h = 46;
  float x = m_screenW / 2 + 30;
  float y = m_screenH / 2;

  //
  // Title bar

  float titleHeight = 20.0f;
  float boxH = numAvailable * (h + 5);

  glShadeModel(GL_SMOOTH);
  glBegin(GL_QUADS);
  glColor4ub(199, 214, 220, 255);
  glVertex2f(x, y - titleHeight);
  glVertex2f(x + w, y - titleHeight);
  glColor4ub(112, 141, 168, 255);
  glVertex2f(x + w, y);
  glVertex2f(x, y);
  glEnd();
  glShadeModel(GL_FLAT);

  g_gameFont.SetRenderShadow(true);
  glColor4ub(255, 255, 150, 30);
  g_gameFont.DrawText2DCenter(x + w / 2.0f, y - titleHeight / 2 + 1, 13, Strings::Get("newcontrols_createnewtask"));
  g_gameFont.DrawText2DCenter(x + w / 2.0f, y - titleHeight / 2 + 1, 13, Strings::Get("newcontrols_createnewtask"));
  g_gameFont.SetRenderShadow(false);

  //
  // Background box

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_red.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(x, y);
  glTexCoord2f(1, 0);
  glVertex2f(x + w, y);
  glTexCoord2f(1, boxH / m_screenH * 1.5);
  glVertex2f(x + w, y + boxH);
  glTexCoord2f(0, boxH / m_screenH * 1.5);
  glVertex2f(x, y + boxH);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  glColor4ub(199, 214, 220, 255);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y - titleHeight);
  glVertex2f(x + w, y - titleHeight);
  glVertex2f(x + w, y + boxH);
  glVertex2f(x, y + boxH);
  glEnd();

  y += titleHeight;

  for (int i = 0; i < numRunnableTasks; ++i)
  {
    int taskType = runnableTaskType[i];
    if (g_app->m_globalWorld->m_research->HasResearch(taskType))
    {
      char tooltipId[128];
      sprintf(tooltipId, "newcontrols_create_%s", GlobalResearch::GetTypeName(taskType).c_str());

      auto zone = NEW ScreenZone("NewTask", Strings::Get(tooltipId), x + 5, y - h / 3, w - 10, h, taskType);
      zone->m_scrollZone = 1;
      m_newScreenZones.PutData(zone);

      //
      // Render task name and F-key shortcut

      glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(x + 70, y + 5, 18, GlobalResearch::GetTypeNameTranslated(taskType));

      glColor4f(0.8f, 0.8f, 1.0f, 1.0f);
      g_gameFont.DrawText2D(x + 70, y + 5, 18, GlobalResearch::GetTypeNameTranslated(taskType));

      //
      // Render the shadow

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_shadow.bmp"));
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
      glDepthMask(false);
      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);

      float iconSize = (h - 6);
      float iconX = x + 10;
      float iconY = y - 12;

      float shadowSize = iconSize * 1.1f;

      glBegin(GL_QUADS);
      glTexCoord2i(0, 1);
      glVertex2f(iconX, iconY);
      glTexCoord2i(1, 1);
      glVertex2f(iconX + shadowSize, iconY);
      glTexCoord2i(1, 0);
      glVertex2f(iconX + shadowSize, iconY + shadowSize);
      glTexCoord2i(0, 0);
      glVertex2f(iconX, iconY + shadowSize);
      glEnd();

      glDisable(GL_TEXTURE_2D);

      //
      // Render the task symbol

      char iconFilename[256];
      sprintf(iconFilename, "icons\\icon_%s.bmp", GlobalResearch::GetTypeName(taskType).c_str());
      int texId = Resource::GetTexture(iconFilename);
      if (texId != -1)
      {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 1);
        glVertex2f(iconX, iconY);
        glTexCoord2i(1, 1);
        glVertex2f(iconX + iconSize, iconY);
        glTexCoord2i(1, 0);
        glVertex2f(iconX + iconSize, iconY + iconSize);
        glTexCoord2i(0, 0);
        glVertex2f(iconX, iconY + iconSize);
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_TEXTURE_2D);
      }
    }

    y += h + 4;
  }
}

void TaskManagerInterfaceIcons::RenderTitleBar()
{
  SetupRenderMatrices(ScreenTaskManager);

  //
  // Render panel

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(0.5f, 0.5f, 1.0f, 0.7f);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_grey.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  float panelW = 80;

  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex2i(m_screenW, -m_screenH);
  glTexCoord2i(1, 0);
  glVertex2i(m_screenW, m_screenH * 2);
  glTexCoord2i(1, 1);
  glVertex2i(m_screenW - panelW, m_screenH * 2);
  glTexCoord2i(0, 1);
  glVertex2i(m_screenW - panelW, -m_screenH);
  glEnd();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDisable(GL_TEXTURE_2D);

  //
  // Render dividing line

  glBegin(GL_LINES);
  glVertex2i(m_screenW - panelW, -m_screenH);
  glVertex2i(m_screenW - panelW, m_screenH * 2);
  glEnd();
}

void TaskManagerInterfaceIcons::RenderRunningTasks()
{
  int numSlots = g_app->m_taskManager->Capacity();

  static float s_iconSize = 45;
  static float s_iconX = 40;
  static float s_iconY = 130;

  float desiredIconSize = 50;
  float desiredIconX = 40;
  float desiredIconY = 130;

  if (m_visible)
  {
    desiredIconSize = 70;
    desiredIconX = 80;
    desiredIconY = 130;
  }

  float factor = g_advanceTime * 10;
  s_iconSize = (s_iconSize * (1 - factor)) + (desiredIconSize * factor);
  s_iconX = (s_iconX * (1 - factor)) + (desiredIconX * factor);
  s_iconY = (s_iconY * (1 - factor)) + (desiredIconY * factor);

  float iconX = s_iconX;
  float iconY = s_iconY;
  float iconSize = s_iconSize;
  float iconGap = 10;
  float shadowOffset = 0;
  float shadowSize = iconSize + 10;

  float iconAlpha = 0.9f;

  //
  // Render shadows for available task slots

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_shadow.bmp"));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  glDepthMask(false);
  glColor4f(iconAlpha, iconAlpha, iconAlpha, 0.0f);

  for (int i = 0; i < numSlots; ++i)
  {
    LegacyVector2 iconCenter(iconX, iconY);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex2f(iconCenter.x - shadowSize / 2 + shadowOffset, iconCenter.y - shadowSize / 2 + shadowOffset);
    glTexCoord2i(1, 1);
    glVertex2f(iconCenter.x + shadowSize / 2 + shadowOffset, iconCenter.y - shadowSize / 2 + shadowOffset);
    glTexCoord2i(1, 0);
    glVertex2f(iconCenter.x + shadowSize / 2 + shadowOffset, iconCenter.y + shadowSize / 2 + shadowOffset);
    glTexCoord2i(0, 0);
    glVertex2f(iconCenter.x - shadowSize / 2 + shadowOffset, iconCenter.y + shadowSize / 2 + shadowOffset);
    glEnd();

    iconY += iconSize;
    iconY += iconGap;
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
  glDepthMask(true);

  iconY = s_iconY;

  //
  // Render our running tasks

  int numTasks = g_app->m_taskManager->m_tasks.Size();

  for (int i = 0; i < numTasks; ++i)
  {
    Task* task = g_app->m_taskManager->m_tasks[i];
    char bmpFilename[256];
    sprintf(bmpFilename, "icons\\icon_%s.bmp", Task::GetTaskName(task->m_type).c_str());
    unsigned int texId = Resource::GetTexture(bmpFilename);

    //
    // Create clickable zone over the task

    char captionId[256];
    sprintf(captionId, "newcontrols_select_%s", Task::GetTaskName(task->m_type).c_str());
    if (task->m_state == Task::StateStarted)
      sprintf(captionId, "newcontrols_place_%s", Task::GetTaskName(task->m_type).c_str());

    auto zone = NEW ScreenZone("SelectTask", Strings::Get(captionId), iconX - iconSize / 2, iconY - iconSize / 2, iconSize,
      iconSize, i);
    m_newScreenZones.PutData(zone);
    zone->m_scrollZone = 2;

    bool invisible = (task->m_state == Task::StateStarted && fmod(g_gameTime, 1.0) < 0.4);

    LegacyVector2 iconCenter(iconX, iconY);

    if (!invisible)
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, texId);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);

      if (task->m_id == g_app->m_taskManager->m_currentTaskId)
        glColor4f(1.0f, 1.0f, 1.0f, iconAlpha);
      else
        glColor4f(1.0f, 1.0f, 1.0f, iconAlpha * 0.3f);

      glBegin(GL_QUADS);
      glTexCoord2i(0, 1);
      glVertex2f(iconCenter.x - iconSize / 2, iconCenter.y - iconSize / 2);
      glTexCoord2i(1, 1);
      glVertex2f(iconCenter.x + iconSize / 2, iconCenter.y - iconSize / 2);
      glTexCoord2i(1, 0);
      glVertex2f(iconCenter.x + iconSize / 2, iconCenter.y + iconSize / 2);
      glTexCoord2i(0, 0);
      glVertex2f(iconCenter.x - iconSize / 2, iconCenter.y + iconSize / 2);
      glEnd();

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_TEXTURE_2D);

      if (task->m_type == GlobalResearch::TypeEngineer)
      {
        if (auto engineer = static_cast<Engineer*>(g_app->m_location->GetEntitySafe(task->m_objId, EntityType::TypeEngineer)))
        {
          auto state = engineer->GetCurrentAction();
          int numSpirits = engineer->GetNumSpirits();

          g_gameFont.SetRenderShadow(true);
          glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
          g_gameFont.DrawText2DCenter(iconCenter.x + 3, iconCenter.y + 25, 7, state);
          g_gameFont.DrawText2DCenter(iconCenter.x + 3, iconCenter.y + 2, 12, "%d", numSpirits);

          g_gameFont.SetRenderShadow(false);
          glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
          g_gameFont.DrawText2DCenter(iconCenter.x + 3, iconCenter.y + 25, 7, state);
          g_gameFont.DrawText2DCenter(iconCenter.x + 3, iconCenter.y + 2, 12, "%d", numSpirits);
        }
      }
      else if (task->m_type == GlobalResearch::TypeSquad)
      {
        auto squad = static_cast<InsertionSquad*>(g_app->m_location->GetUnit(task->m_objId));
        if (squad)
        {
          int numSquaddies = squad->NumAliveEntities();

          glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
          g_gameFont.SetRenderShadow(true);
          g_gameFont.DrawText2D(iconCenter.x + 11, iconCenter.y + 5, 10, "x%d", numSquaddies);

          glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
          g_gameFont.SetRenderShadow(false);
          g_gameFont.DrawText2D(iconCenter.x + 11, iconCenter.y + 5, 10, "x%d", numSquaddies);
        }
      }

      //
      // Render weapon selection if we're a squad
      // Render mode selection if we're an officer or armour
      // Render compass if we are an object

      LegacyVector3 taskWorldPos;
      Unit* unit = g_app->m_location->GetUnit(task->m_objId);
      Entity* entity = g_app->m_location->GetEntity(task->m_objId);
      if (unit)
        taskWorldPos = unit->m_centerPos;
      if (entity)
        taskWorldPos = entity->m_pos;
      float compassSize = iconSize * 0.75f;

      if (taskWorldPos != g_zeroVector)
        RenderCompass(iconCenter.x, iconCenter.y, taskWorldPos, task->m_id == g_app->m_taskManager->m_currentTaskId, compassSize);

      if (task->m_state != Task::StateStarted && m_visible && task->m_id == g_app->m_taskManager->m_currentTaskId)
      {
        if (task->m_type == GlobalResearch::TypeSquad)
        {
          LList<int> availableWeapons;
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeGrenade))
            availableWeapons.PutData(GlobalResearch::TypeGrenade);
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeRocket))
            availableWeapons.PutData(GlobalResearch::TypeRocket);
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeAirStrike))
            availableWeapons.PutData(GlobalResearch::TypeAirStrike);
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeController))
            availableWeapons.PutData(GlobalResearch::TypeController);

          auto squad = static_cast<InsertionSquad*>(unit);
          int currentWeapon = -1;
          if (squad)
            currentWeapon = squad->m_weaponType;

          float weaponSize = iconSize * 0.5f;
          float weaponGap = weaponSize * 0.15f;
          float weaponX = iconCenter.x + iconSize / 1.2f;
          float weaponY = iconCenter.y + iconSize / 5;

          glEnable(GL_TEXTURE_2D);

          if (availableWeapons.Size() > 0)
            zone->m_subZones = true;

          for (int j = 0; j < availableWeapons.Size(); ++j)
          {
            int weaponType = availableWeapons[j];
            sprintf(bmpFilename, "icons\\icon_%s.bmp", Task::GetTaskName(weaponType).c_str());
            texId = Resource::GetTexture(bmpFilename);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
            glColor4f(0.9f, 0.9f, 0.9f, 0.0f);
            glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_shadow.bmp"));
            glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex2f(weaponX - weaponSize / 2, weaponY - weaponSize / 2);
            glTexCoord2i(1, 1);
            glVertex2f(weaponX + weaponSize / 2, weaponY - weaponSize / 2);
            glTexCoord2i(1, 0);
            glVertex2f(weaponX + weaponSize / 2, weaponY + weaponSize / 2);
            glTexCoord2i(0, 0);
            glVertex2f(weaponX - weaponSize / 2, weaponY + weaponSize / 2);
            glEnd();

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            glBindTexture(GL_TEXTURE_2D, texId);
            glColor4f(1.0f, 1.0f, 1.0f, iconAlpha * 0.3f);
            if (weaponType == currentWeapon)
              glColor4f(1.0f, 1.0f, 1.0f, iconAlpha);

            glBegin(GL_QUADS);
            glTexCoord2i(0, 1);
            glVertex2f(weaponX - weaponSize / 2, weaponY - weaponSize / 2);
            glTexCoord2i(1, 1);
            glVertex2f(weaponX + weaponSize / 2, weaponY - weaponSize / 2);
            glTexCoord2i(1, 0);
            glVertex2f(weaponX + weaponSize / 2, weaponY + weaponSize / 2);
            glTexCoord2i(0, 0);
            glVertex2f(weaponX - weaponSize / 2, weaponY + weaponSize / 2);
            glEnd();

            char id[256];
            sprintf(id, "newcontrols_select_%s", Task::GetTaskName(weaponType).c_str());
            auto screenZone = NEW ScreenZone("SelectWeapon", Strings::Get(id), weaponX - weaponSize / 2,
              weaponY - weaponSize / 2, weaponSize, weaponSize, weaponType);
            m_newScreenZones.PutData(screenZone);
            screenZone->m_scrollZone = 3;

            weaponX += weaponSize;
            weaponX += weaponGap;
          }

          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glDisable(GL_TEXTURE_2D);
        }
      }
    }

    //
    // Render delete icon

    if (m_visible)
    {
      float deleteX = iconCenter.x - iconSize / 1.7f;
      float deleteY = iconCenter.y - iconSize / 1.7f;
      float deleteSize = iconSize * 0.4f;

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_shadow.bmp"));
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
      glDepthMask(false);
      glColor4f(0.9f, 0.9f, 0.9f, 0.0f);

      glBegin(GL_QUADS);
      glTexCoord2i(0, 0);
      glVertex2f(deleteX - deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      glTexCoord2i(1, 0);
      glVertex2f(deleteX + deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      glTexCoord2i(1, 1);
      glVertex2f(deleteX + deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      glTexCoord2i(0, 1);
      glVertex2f(deleteX - deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      glEnd();

      glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_delete.bmp"));
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

      glBegin(GL_QUADS);
      glTexCoord2i(0, 0);
      glVertex2f(deleteX - deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      glTexCoord2i(1, 0);
      glVertex2f(deleteX + deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      glTexCoord2i(1, 1);
      glVertex2f(deleteX + deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      glTexCoord2i(0, 1);
      glVertex2f(deleteX - deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      glEnd();

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_TEXTURE_2D);

      char captionId[256];
      sprintf(captionId, "newcontrols_delete_%s", Task::GetTaskName(task->m_type).c_str());

      auto screenZone = NEW ScreenZone("DeleteTask", Strings::Get(captionId), deleteX - deleteSize / 2.0f, deleteY - deleteSize / 2.0f,
        deleteSize, deleteSize, task->m_id);
      m_newScreenZones.PutData(screenZone);
    }

    iconY += iconSize;
    iconY += iconGap;
  }
}

void TaskManagerInterfaceIcons::RenderCompass(float _screenX, float _screenY, const LegacyVector3& _worldPos, bool _selected,
  float _size)
{
  g_app->m_renderer->SetupMatricesFor3D();
  float screenH = ClientEngine::OutputSize().Height;
  float screenW = ClientEngine::OutputSize().Width;

  //
  // Project worldPos into screen co-ordinates
  // Is it on screen or not?

  float screenX, screenY;
  g_app->m_camera->Get2DScreenPos(_worldPos, &screenX, &screenY);
  screenY = screenH - screenY;

  LegacyVector3 toCam = g_app->m_camera->GetPos() - _worldPos;
  float angle = toCam * g_app->m_camera->GetFront();
  LegacyVector3 rotationVector = toCam ^ g_app->m_camera->GetFront();

  LegacyVector2 compassVector;

  if (angle <= 0.0f && screenX >= 0 && screenX < screenW && screenY >= 0 && screenY < screenH)
  {
    // _pos is onscreen
    screenX *= (m_screenW / screenW);
    screenY *= (m_screenH / screenH);

    compassVector.Set(screenX - _screenX, screenY - _screenY);
    compassVector.Normalize();
  }
  else
  {
    // _pos is offscreen
    LegacyVector3 rayStart, rayDir;
    g_app->m_camera->GetClickRay(_screenX * screenW / m_screenW, _screenY * screenH / m_screenH, &rayStart, &rayDir);
    LegacyVector3 camPos = rayStart + rayDir * 1000;
    LegacyVector3 camToTarget = (_worldPos - camPos).SetLength(100);

    float posX, posY;
    g_app->m_camera->Get2DScreenPos(camPos + camToTarget, &posX, &posY);
    posY = screenH - posY;
    posX *= (m_screenW / screenW);
    posY *= (m_screenH / screenH);

    compassVector.Set(posX - _screenX, posY - _screenY);
    compassVector.Normalize();
  }

  //
  // Render the compass

  LegacyVector2 screenPos(_screenX - 0.5f, _screenY - 0.5f);
  LegacyVector2 compassRight = compassVector;
  float temp = compassRight.x;
  compassRight.x = compassRight.y;
  compassRight.y = temp * -1.0f;

  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\compass.bmp", true, false));
  glEnable(GL_TEXTURE_2D);

  g_app->m_renderer->SetupMatricesFor2D();
  SetupRenderMatrices(ScreenTaskManager);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  glColor4f(0.8f, 0.8f, 0.8f, 0.0f);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2fv((screenPos - compassRight * _size + compassVector * _size).GetData());
  glTexCoord2i(1, 1);
  glVertex2fv((screenPos + compassRight * _size + compassVector * _size).GetData());
  glTexCoord2i(1, 0);
  glVertex2fv((screenPos + compassRight * _size - compassVector * _size).GetData());
  glTexCoord2i(0, 0);
  glVertex2fv((screenPos - compassRight * _size - compassVector * _size).GetData());
  glEnd();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  if (_selected)
    glColor4f(1.0f, 1.0f, 0.3f, 0.8f);
  else
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2fv((screenPos - compassRight * _size + compassVector * _size).GetData());
  glTexCoord2i(1, 1);
  glVertex2fv((screenPos + compassRight * _size + compassVector * _size).GetData());
  glTexCoord2i(1, 0);
  glVertex2fv((screenPos + compassRight * _size - compassVector * _size).GetData());
  glTexCoord2i(0, 0);
  glVertex2fv((screenPos - compassRight * _size - compassVector * _size).GetData());
  glEnd();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
}

void TaskManagerInterfaceIcons::RenderObjectives()
{
  SetupRenderMatrices(ScreenObjectives);

  //
  // Render title

  //g_gameFont.SetRenderOutline(true);
  glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
  //g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, Strings::Get("taskmanager_objectives"));
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  //g_gameFont.SetRenderOutline(false);
  //g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, Strings::Get("taskmanager_objectives"));

  //
  // Render background fill

  float panelW = 81;
  float textX = 50;
  float completeX = m_screenW - 300;
  float textY = 100;
  float textH = 25;

  float boxX = 30;
  float boxW = m_screenW - boxX * 2 - panelW;

  //
  // Render primary objectives

  float mouseX = g_target->X();
  float mouseY = g_target->Y();

  ConvertMousePosition(mouseX, mouseY);

  for (int o = 0; o < 2; ++o)
  {
    LList<GlobalEventCondition*>* objectives = nullptr;
    if (o == 0)
      objectives = &g_app->m_location->m_levelFile->m_primaryObjectives;
    else
      objectives = &g_app->m_location->m_levelFile->m_secondaryObjectives;

    int numObjectives = objectives->Size();
    if (numObjectives == 0)
      continue;

    //
    // Title bar

    float boxY = textY - textH;
    float boxH = numObjectives * (textH * 1.5f) + textH * 0.75f;
    float titleHeight = 20.0f;

    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUADS);
    glColor4ub(199, 214, 220, 255);
    glVertex2f(boxX, boxY - titleHeight);
    glVertex2f(boxX + boxW, boxY - titleHeight);
    glColor4ub(112, 141, 168, 255);
    glVertex2f(boxX + boxW, boxY);
    glVertex2f(boxX, boxY);
    glEnd();
    glShadeModel(GL_FLAT);

    g_gameFont.SetRenderShadow(true);
    glColor4ub(255, 255, 150, 30);
    auto theString = (o == 0 ? Strings::Get("taskmanager_primarys") : Strings::Get("taskmanager_secondarys"));
    g_gameFont.DrawText2DCenter(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
    g_gameFont.DrawText2DCenter(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
    g_gameFont.SetRenderShadow(false);

    //
    // Background box

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_red.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(boxX, boxY);
    glTexCoord2f(1, 0);
    glVertex2f(boxX + boxW, boxY);
    glTexCoord2f(1, boxH / m_screenH * 1.5);
    glVertex2f(boxX + boxW, boxY + boxH);
    glTexCoord2f(0, boxH / m_screenH * 1.5);
    glVertex2f(boxX, boxY + boxH);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glColor4ub(199, 214, 220, 255);
    glBegin(GL_LINE_LOOP);
    glVertex2f(boxX, boxY - titleHeight);
    glVertex2f(boxX + boxW, boxY - titleHeight);
    glVertex2f(boxX + boxW, boxY + boxH);
    glVertex2f(boxX, boxY + boxH);
    glEnd();

    for (int i = 0; i < numObjectives; ++i)
    {
      float objectiveId = i + 10;
      if (o == 1)
        objectiveId *= -1;
      auto zone = NEW ScreenZone("Objective", Strings::Get("help_explainobjective"), boxX + 10, m_screenH + textY - textH / 2,
        boxW - 20, textH * 1.5f, objectiveId);
      zone->m_scrollZone = 1;
      m_newScreenZones.PutData(zone);

      GlobalEventCondition* condition = objectives->GetData(i);
      bool completed = condition->Evaluate();

      auto descriptor = Strings::Get(condition->m_stringId);

      //g_gameFont.SetRenderOutline(true);
      glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(textX, textY, textH * 0.65f, descriptor);
      //g_gameFont.SetRenderOutline(false);
      if (completed)
        glColor4f(0.8f, 0.8f, 1.0f, 1.0f);
      else
        glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
      g_gameFont.DrawText2D(textX, textY, textH * 0.65f, descriptor);

      auto completedString = (completed ? Strings::Get("taskmanager_complete") : Strings::Get("taskmanager_incomplete"));
      //g_gameFont.SetRenderOutline(true);
      glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(completeX, textY, textH * 0.75f, completedString);
      //g_gameFont.SetRenderOutline(false);
      if (completed)
        glColor4f(0.8f, 0.8f, 1.0f, 1.0f);
      else
        glColor4f(1.0f, 0.2f, 0.2f, 1.0f);
      g_gameFont.DrawText2D(completeX, textY, textH * 0.75f, completedString);

      if (condition->m_type == GlobalEventCondition::BuildingOnline)
      {
        if (Building* building = g_app->m_location->GetBuilding(condition->m_id))
        {
          auto objectiveCounter = building->GetObjectiveCounter();
          g_gameFont.DrawText2D(completeX, textY + textH * 0.75f, textH / 3, objectiveCounter);
        }
      }

      textY += textH * 1.5f;
    }

    textY += 70;
  }

  //
  // Render nav arrows

  if (m_screenY > 0.9f)
  {
    unsigned alpha = (fmodf(g_gameTime, 2.0f) < 1.0f ? 155 : 255);

    glColor4ub(199, 214, 220, alpha);

    glBegin(GL_TRIANGLES);
    glVertex2f(m_screenW - 40, 10);
    glVertex2f(m_screenW - 20, 30);
    glVertex2f(m_screenW - 60, 30);
    glEnd();

    auto zoneLeft = NEW ScreenZone("ScreenUp", Strings::Get("newcontrols_showtaskmanager"), m_screenW - 60, m_screenH + 10, 40,
      20, -1);

    m_newScreenZones.PutData(zoneLeft);
  }
}

void TaskManagerInterfaceIcons::RenderResearch()
{
  SetupRenderMatrices(ScreenResearch);

  //
  // Render title text

  //g_gameFont.SetRenderOutline(true);
  glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
  //g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, Strings::Get("taskmanager_research"));
  //g_gameFont.SetRenderOutline(false);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  //g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, Strings::Get("taskmanager_research"));
  //g_gameFont.DrawText2DCenter( m_screenW/2.0f, 80, 20, "ResearchPoints : %d", g_app->m_globalWorld->m_research->m_researchPoints );

  float mouseX = g_target->X();
  float mouseY = g_target->Y();
  ConvertMousePosition(mouseX, mouseY);

  int numItemsResearched = 0;
  for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(i))
      ++numItemsResearched;
  }

  //
  // Title bar

  float panelW = 80;
  float textH = 26;
  float boxX = 30;
  float boxW = m_screenW - boxX * 2 - panelW;
  float textY = 70;
  float boxY = textY - textH;
  float boxH = numItemsResearched * (textH * 1.7f) + textH * 1.0f;
  float titleHeight = 20.0f;

  glShadeModel(GL_SMOOTH);
  glBegin(GL_QUADS);
  glColor4ub(199, 214, 220, 255);
  glVertex2f(boxX, boxY - titleHeight);
  glVertex2f(boxX + boxW, boxY - titleHeight);
  glColor4ub(112, 141, 168, 255);
  glVertex2f(boxX + boxW, boxY);
  glVertex2f(boxX, boxY);
  glEnd();
  glShadeModel(GL_FLAT);

  g_gameFont.SetRenderShadow(true);
  glColor4ub(255, 255, 150, 30);
  auto theString = Strings::Get("taskmanager_research");
  g_gameFont.DrawText2DCenter(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
  g_gameFont.DrawText2DCenter(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
  g_gameFont.SetRenderShadow(false);

  //
  // Background box

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_red.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(boxX, boxY);
  glTexCoord2f(1, 0);
  glVertex2f(boxX + boxW, boxY);
  glTexCoord2f(1, boxH / m_screenH * 1.5);
  glVertex2f(boxX + boxW, boxY + boxH);
  glTexCoord2f(0, boxH / m_screenH * 1.5);
  glVertex2f(boxX, boxY + boxH);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  glColor4ub(199, 214, 220, 255);
  glBegin(GL_LINE_LOOP);
  glVertex2f(boxX, boxY - titleHeight);
  glVertex2f(boxX + boxW, boxY - titleHeight);
  glVertex2f(boxX + boxW, boxY + boxH);
  glVertex2f(boxX, boxY + boxH);
  glEnd();

  //
  // Render each item

  float iconY = textY - 20;
  float totalSize = (boxH - 20) / static_cast<float>(numItemsResearched);
  totalSize = std::min(totalSize, 50.0f);

  for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(i))
    {
      float iconX = 50;
      float gapSize = 3;
      float iconSize = totalSize - gapSize;

      //
      // Render selection boxes

      if (g_app->m_globalWorld->m_research->m_currentResearch == i)
      {
        glColor4f(0.05f, 0.05f, 0.5f, 0.4f);
        glBegin(GL_QUADS);
        glVertex2i(40, iconY - 1);
        glVertex2i(m_screenW - 120, iconY - 1);
        glVertex2i(m_screenW - 120, iconY + iconSize + 1);
        glVertex2i(40, iconY + iconSize + 1);
        glEnd();
        glColor4f(0.5f, 0.5f, 0.7f, 0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(40, iconY - 1);
        glVertex2i(m_screenW - 120, iconY - 1);
        glVertex2i(m_screenW - 120, iconY + iconSize + 1);
        glVertex2i(40, iconY + iconSize + 1);
        glEnd();
      }

      char tooltipId[256];
      sprintf(tooltipId, "newcontrols_research_%s", GlobalResearch::GetTypeName(i).c_str());
      auto zone = NEW ScreenZone("Research", Strings::Get(tooltipId), 40, -m_screenH + iconY, m_screenW - 160, iconSize, i);
      zone->m_scrollZone = 1;
      m_newScreenZones.PutData(zone);

      //
      // Render the shadow

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_shadow.bmp"));
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
      glDepthMask(false);
      glColor4f(0.5f, 0.5f, 0.5f, 0.0f);

      float shadowSize = iconSize * 1.1f;

      glBegin(GL_QUADS);
      glTexCoord2i(0, 1);
      glVertex2f(iconX, iconY);
      glTexCoord2i(1, 1);
      glVertex2f(iconX + shadowSize, iconY);
      glTexCoord2i(1, 0);
      glVertex2f(iconX + shadowSize, iconY + shadowSize);
      glTexCoord2i(0, 0);
      glVertex2f(iconX, iconY + shadowSize);
      glEnd();

      glDisable(GL_TEXTURE_2D);

      //
      // Render the task symbol

      char iconFilename[256];
      sprintf(iconFilename, "icons\\icon_%s.bmp", GlobalResearch::GetTypeName(i).c_str());
      int texId = Resource::GetTexture(iconFilename);
      if (texId != -1)
      {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 1);
        glVertex2f(iconX, iconY);
        glTexCoord2i(1, 1);
        glVertex2f(iconX + iconSize, iconY);
        glTexCoord2i(1, 0);
        glVertex2f(iconX + iconSize, iconY + iconSize);
        glTexCoord2i(0, 0);
        glVertex2f(iconX, iconY + iconSize);
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_TEXTURE_2D);
      }

      //
      // Render research progress

      int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel(i);
      int researchProgress = g_app->m_globalWorld->m_research->CurrentProgress(i);

      //g_gameFont.SetRenderOutline(true);
      glColor4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(iconX + iconSize + 10, iconY + iconSize / 2, 20, GlobalResearch::GetTypeNameTranslated(i));
      //g_gameFont.SetRenderOutline(false);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      g_gameFont.DrawText2D(iconX + iconSize + 10, iconY + iconSize / 2, 20, GlobalResearch::GetTypeNameTranslated(i));

      float x = 300;
      const float y = iconY + iconSize * 0.25f;
      const float h = iconSize * 0.4f;

      glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_grey.bmp"));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      for (int level = 1; level < 4; ++level)
      {
        float boxScale = 0.85f;

        int requiredProgress = g_app->m_globalWorld->m_research->RequiredProgress(level);

        float shadowOffset = 2.0f;
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x + requiredProgress * boxScale + shadowOffset, y);
        glVertex2i(x + requiredProgress * boxScale + shadowOffset, y + h + shadowOffset);
        glVertex2i(x, y + h + shadowOffset);
        glEnd();

        if (researchLevel > level)
        {
          glColor4f(0.7f, 0.7f, 0.9f, 1.0f);
          glEnable(GL_TEXTURE_2D);
          glBegin(GL_QUADS);
          glTexCoord2i(0, 0);
          glVertex2i(x, y);
          glTexCoord2i(10, 0);
          glVertex2i(x + requiredProgress * boxScale, y);
          glTexCoord2i(10, 1);
          glVertex2i(x + requiredProgress * boxScale, y + h);
          glTexCoord2i(0, 1);
          glVertex2i(x, y + h);
          glEnd();
          float alpha = 1.0f;
          if (g_app->m_globalWorld->m_research->m_currentResearch == i)
            alpha = fabs(sinf(g_gameTime * 2));
          glColor4f(0.9f, 0.9f, 1.0f, alpha);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE);
          glBegin(GL_QUADS);
          glTexCoord2i(0, 0);
          glVertex2i(x, y);
          glTexCoord2i(10, 0);
          glVertex2i(x + requiredProgress * boxScale, y);
          glTexCoord2i(10, 1);
          glVertex2i(x + requiredProgress * boxScale, y + h);
          glTexCoord2i(0, 1);
          glVertex2i(x, y + h);
          glTexCoord2i(0, 0);
          glVertex2i(x, y);
          glTexCoord2i(10, 0);
          glVertex2i(x + requiredProgress * boxScale, y);
          glTexCoord2i(10, 1);
          glVertex2i(x + requiredProgress * boxScale, y + h);
          glTexCoord2i(0, 1);
          glVertex2i(x, y + h);
          glEnd();
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glDisable(GL_TEXTURE_2D);
        }

        if (researchLevel == level && researchProgress > 0)
        {
          glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
          glEnable(GL_TEXTURE_2D);
          glBegin(GL_QUADS);
          glTexCoord2i(0, 0);
          glVertex2i(x, y);
          glTexCoord2i(10, 0);
          glVertex2i(x + researchProgress * boxScale, y);
          glTexCoord2i(10, 1);
          glVertex2i(x + researchProgress * boxScale, y + h);
          glTexCoord2i(0, 1);
          glVertex2i(x, y + h);
          glEnd();
          if (g_app->m_globalWorld->m_research->m_currentResearch == i)
          {
            float alpha = fabs(sinf(g_gameTime * 2));
            glColor4f(0.9f, 0.9f, 1.0f, alpha);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBegin(GL_QUADS);
            glTexCoord2i(0, 0);
            glVertex2i(x, y);
            glTexCoord2i(10, 0);
            glVertex2i(x + researchProgress * boxScale, y);
            glTexCoord2i(10, 1);
            glVertex2i(x + researchProgress * boxScale, y + h);
            glTexCoord2i(0, 1);
            glVertex2i(x, y + h);
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          }

          glDisable(GL_TEXTURE_2D);
        }

        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

        glBegin(GL_LINE_LOOP);
        glVertex2i(x, y);
        glVertex2i(x + requiredProgress * boxScale, y);
        glVertex2i(x + requiredProgress * boxScale, y + h);
        glVertex2i(x, y + h);
        glEnd();

        x += requiredProgress * boxScale;
        x += 3;
      }

      float texX = 630;

      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      g_gameFont.DrawText2D(texX, iconY + iconSize / 2, 12, "v%d.0", researchLevel);

      iconY += totalSize;
    }
  }

  //
  // Render navigation arrows

  if (m_screenY < 0.1f)
  {
    unsigned alpha = (fmodf(g_gameTime, 2.0f) < 1.0f ? 155 : 255);

    glColor4ub(199, 214, 220, alpha);

    glBegin(GL_TRIANGLES);
    glVertex2f(m_screenW - 40, m_screenH - 30);
    glVertex2f(m_screenW - 20, m_screenH - 50);
    glVertex2f(m_screenW - 60, m_screenH - 50);
    glEnd();

    auto zoneRight = NEW ScreenZone("ScreenDown", Strings::Get("newcontrols_showtaskmanager"), m_screenW - 60, -50, 40, 20, -1);
    zoneRight->m_scrollZone = -1;
    m_newScreenZones.PutData(zoneRight);
  }
}

bool TaskManagerInterfaceIcons::ControlEvent(TMControl _type)
{
  switch (_type)
  {
  case TMTerminate:
    return g_inputManager->controlEvent(ControlIconsTaskManagerEndTask);
  case TMDisplay:
    return g_inputManager->controlEvent(ControlIconsTaskManagerDisplay);
  default:
    return false;
  }
}

void TaskManagerInterfaceIcons::AdvanceQuickUnit()
{
  if (!m_quickUnitVisible)
    return;

  int numRunnableTasks = 4;
  int runnableTaskType[] = {
    GlobalResearch::TypeSquad,
    GlobalResearch::TypeEngineer,
    GlobalResearch::TypeOfficer,
    GlobalResearch::TypeArmour
  };

  int numAvailable = 0;
  for (int i = 0; i < numRunnableTasks; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      numAvailable++;
  }

  if (numAvailable == 0)
  {
    m_quickUnitVisible = false;
    return;
  }

  for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
    m_quickUnitButtons[i]->Advance();

  bool showUnitName = false;

  if (numAvailable > 1 && m_quickUnitDirection == 0)
  {
    InputDetails cameraDetails, targetDetails;
    g_inputManager->controlEvent(ControlTargetMove, targetDetails);
    g_inputManager->controlEvent(ControlCameraMove, cameraDetails);

    bool right = (g_inputManager->controlEvent(ControlUnitCycleLeft) || cameraDetails.x < -10 || targetDetails.x < -10 ||
      g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitLeft));

    bool left = (g_inputManager->controlEvent(ControlUnitCycleRight) || cameraDetails.x > 10 || targetDetails.x > 10 ||
      g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitRight));

    if (left)
    {
      while (true)
      {
        m_currentQuickUnit--;
        if (m_currentQuickUnit < 0)
          m_currentQuickUnit = numRunnableTasks - 1;
        if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[m_currentQuickUnit]))
          break;
      }

      for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
        m_quickUnitButtons[i]->m_movable = true;
      m_quickUnitDirection = -1;
      showUnitName = true;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
    }

    if (right)
    {
      while (true)
      {
        m_currentQuickUnit++;
        if (m_currentQuickUnit >= numRunnableTasks)
          m_currentQuickUnit = 0;
        if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[m_currentQuickUnit]))
          break;
      }

      for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
        m_quickUnitButtons[i]->m_movable = true;
      m_quickUnitDirection = 1;
      showUnitName = true;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
    }

    if (showUnitName)
    {
      int taskId = GetQuickUnitTask(2 - m_quickUnitDirection);
      if (taskId != -1)
        g_app->m_taskManagerInterface->SetCurrentMessage(MessageSuccess, taskId, 3.0f);
    }

    if (g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitCreate))
    {
      int taskId = GetQuickUnitTask();
      if (taskId != -1)
      {
        g_app->m_networkClient->RequestRunProgram(g_app->m_location->GetMyTeam()->m_teamId, taskId);
        g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);
        DestroyQuickUnitInterface();
      }
    }
  }
  else if (m_quickUnitDirection != 0)
  {
    bool stop = true;
    for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
    {
      if (m_quickUnitButtons[i]->m_movable)
        stop = false;
    }
    if (stop)
      m_quickUnitDirection = 0;
  }
}

int TaskManagerInterfaceIcons::GetQuickUnitTask(int _position)
{
  for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
  {
    if (m_quickUnitButtons[i]->m_positionId == _position)
      return m_quickUnitButtons[i]->m_taskId;
  }
  return -1;
}

bool TaskManagerInterfaceIcons::AdviseCreateControlHelpBlue()
{
  return m_currentScreenZone != -1 && ButtonHeld() && g_inputManager->controlEvent(ControlIconsTaskManagerDisplayPressed) && g_app->
    m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity();
}

bool TaskManagerInterfaceIcons::AdviseCreateControlHelpGreen()
{
  ScreenZone* screenZone = nullptr;

  return m_currentScreenZone != -1 && g_app->m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity() && (screenZone =
    m_screenZones[m_currentScreenZone]) && screenZone->m_name == "NewTask";
}

bool TaskManagerInterfaceIcons::AdviseOverSelectableZone()
{
  ScreenZone* screenZone = nullptr;

  return m_currentScreenZone != -1 && (screenZone = m_screenZones[m_currentScreenZone]) && screenZone->m_name != "NewTask";
}

bool TaskManagerInterfaceIcons::AdviseCloseControlHelp()
{
  if (!m_visible)
    return false;

  if (m_currentScreenZone != -1 && ButtonHeld() && g_inputManager->controlEvent(ControlIconsTaskManagerDisplayPressed))
    return false;

  return true;
}

void TaskManagerInterfaceIcons::CreateQuickUnitInterface()
{
  int numRunnableTasks = 4;
  int runnableTaskType[] = {
    GlobalResearch::TypeOfficer,
    GlobalResearch::TypeEngineer,
    GlobalResearch::TypeSquad,
    GlobalResearch::TypeArmour
  };

  int numAvailable = 0;
  for (int i = 0; i < numRunnableTasks; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      numAvailable++;
  }

  while (m_quickUnitButtons.Size() < 5)
  {
    for (int i = 0; i < numRunnableTasks; ++i)
    {
      if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      {
        bool buttonFound = false;
        for (int j = 0; j < m_quickUnitButtons.Size(); ++j)
        {
          if (m_quickUnitButtons[j]->m_taskId == runnableTaskType[i])
          {
            buttonFound = true;
            break;
          }
        }

        if (!buttonFound || (buttonFound && m_quickUnitButtons.Size() >= numAvailable && m_quickUnitButtons.Size() < 5))
        {
          auto button = NEW QuickUnitButton();
          button->m_taskId = runnableTaskType[i];
          m_quickUnitButtons.PutData(button);
        }
      }

      if (m_quickUnitButtons.Size() == 5)
        break;
    }
  }
  m_quickUnitVisible = true;
  if (m_currentQuickUnit == -1)
    m_currentQuickUnit = 0;
  g_app->m_location->GetMyTeam()->SelectUnit(-1, -1, -1);
}

void TaskManagerInterfaceIcons::DestroyQuickUnitInterface()
{
  m_quickUnitButtons.EmptyAndDelete();
  m_quickUnitVisible = false;
  m_currentQuickUnit = -1;
  m_quickUnitDirection = 0;
}

QuickUnitButton::QuickUnitButton()
  : m_taskId(-1),
  m_positionId(-1),
  m_size(0.0f),
  m_x(-1),
  m_y(-1),
  m_alpha(0.9f),
  m_movable(false)
{
  float iconSize = 100.0f;
  float iconGap = 10.0f;
  float iconX = (ClientEngine::OutputSize().Width / 2.0f);// - iconSize - iconGap;
  float iconY = ClientEngine::OutputSize().Height - iconSize - iconGap;

  static LegacyVector2 buttonPositions[5] = {
    LegacyVector2(iconX - iconSize * 2 - iconGap * 2, iconY),
    LegacyVector2(iconX - iconSize - iconGap, iconY),
    LegacyVector2(iconX, iconY),
    LegacyVector2(iconX + iconSize + iconGap, iconY),
    LegacyVector2(iconX + iconSize * 2 + iconGap * 2, iconY)
  };

  auto tm = static_cast<TaskManagerInterfaceIcons*>(g_app->m_taskManagerInterface);
  m_positionId = tm->m_quickUnitButtons.Size();
  if (m_positionId == 0 || m_positionId == 4)
    m_alpha = 0.0f;

  m_x = buttonPositions[m_positionId].x;
  m_y = buttonPositions[m_positionId].y;
}

void QuickUnitButton::Advance()
{
  auto tm = static_cast<TaskManagerInterfaceIcons*>(g_app->m_taskManagerInterface);
  int direction = tm->m_quickUnitDirection;

  if (direction == 0)
    return;

  float iconSize = 100.0f;
  float iconGap = 10.0f;
  float iconX = (ClientEngine::OutputSize().Width / 2.0f);// - iconSize - iconGap;
  float iconY = ClientEngine::OutputSize().Height - iconSize - iconGap;

  static LegacyVector2 buttonPositions[5] = {
    LegacyVector2(iconX - iconSize * 2 - iconGap * 2, iconY),
    LegacyVector2(iconX - iconSize - iconGap, iconY),
    LegacyVector2(iconX, iconY),
    LegacyVector2(iconX + iconSize + iconGap, iconY),
    LegacyVector2(iconX + iconSize * 2 + iconGap * 2, iconY)
  };

  if (direction != 0 && m_movable)
  {
    int nextPos = m_positionId + direction;
    if (nextPos < 0)
      nextPos = 4;
    if (nextPos > 4)
      nextPos = 0;

    if (nextPos == 0 || nextPos == 4)
    {
      int distance = m_x - buttonPositions[nextPos].x;
      if (abs(distance) > iconSize + iconGap)
        m_x = buttonPositions[nextPos].x - direction * iconSize;

      if (nextPos == 0 && direction == -1)
        m_alpha -= 0.09f;
      else if (nextPos == 4 && direction == 1)
        m_alpha -= 0.09f;
    }
    else if (nextPos == 1 && direction == 1)
    {
      m_alpha += 0.09f;
      m_alpha = std::min(m_alpha, 0.9f);
    }
    else if (nextPos == 3 && direction == -1)
    {
      m_alpha += 0.09f;
      m_alpha = std::min(m_alpha, 0.9f);
    }

    m_x += direction * 10;

    if (m_x == buttonPositions[nextPos].x)
    {
      m_movable = false;
      m_positionId = nextPos;
      m_x = buttonPositions[m_positionId].x;
    }
  }
}

void QuickUnitButton::Render()
{
  float iconSize = 100.0f;
  float iconAlpha = m_alpha;
  float shadowOffset = 0;
  float shadowSize = iconSize + 10;

  auto iconCenter = LegacyVector2(m_x, m_y);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("icons\\icon_shadow.bmp"));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  glDepthMask(false);
  glColor4f(iconAlpha, iconAlpha, iconAlpha, 0.0f);
  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2f(iconCenter.x - shadowSize / 2 + shadowOffset, iconCenter.y - shadowSize / 2 + shadowOffset);
  glTexCoord2i(1, 1);
  glVertex2f(iconCenter.x + shadowSize / 2 + shadowOffset, iconCenter.y - shadowSize / 2 + shadowOffset);
  glTexCoord2i(1, 0);
  glVertex2f(iconCenter.x + shadowSize / 2 + shadowOffset, iconCenter.y + shadowSize / 2 + shadowOffset);
  glTexCoord2i(0, 0);
  glVertex2f(iconCenter.x - shadowSize / 2 + shadowOffset, iconCenter.y + shadowSize / 2 + shadowOffset);
  glEnd();

  char bmpFilename[256];
  if (m_taskId == -1)
    sprintf(bmpFilename, "icons\\icon_notask.bmp");
  else
    sprintf(bmpFilename, "icons\\icon_%s.bmp", Task::GetTaskName(m_taskId).c_str());
  unsigned int texId = Resource::GetTexture(bmpFilename);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texId);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  glColor4f(1.0f, 1.0f, 1.0f, iconAlpha);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2f(iconCenter.x - iconSize / 2, iconCenter.y - iconSize / 2);
  glTexCoord2i(1, 1);
  glVertex2f(iconCenter.x + iconSize / 2, iconCenter.y - iconSize / 2);
  glTexCoord2i(1, 0);
  glVertex2f(iconCenter.x + iconSize / 2, iconCenter.y + iconSize / 2);
  glTexCoord2i(0, 0);
  glVertex2f(iconCenter.x - iconSize / 2, iconCenter.y + iconSize / 2);
  glEnd();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
}
