#include "pch.h"
#include "taskmanager.h"
#include "GameApp.h"
#include "RoutingSystem.h"
#include "camera.h"
#include "NetworkClient.h"
#include "darwinian.h"
#include "entity_grid.h"
#include "gamecursor.h"
#include "global_world.h"
#include "insertion_squad.h"

#include "location.h"
#include "math_utils.h"
#include "officer.h"
#include "ParticleSystem.h"
#include "preferences.h"
#include "prefs_other_window.h"
#include "researchitem.h"
#include "soundsystem.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "trunkport.h"
#include "unit.h"

Task::Task()
  : m_id(-1),
    m_type(GlobalResearch::TypeSquad),
    m_state(StateIdle),
    m_route(nullptr) {}

Task::~Task() { delete m_route; }

void Task::Start() { m_state = StateStarted; }

void Task::Target(const LegacyVector3& _pos)
{
  switch (m_type)
  {
    case GlobalResearch::TypeSquad:
      TargetSquad(_pos);
      break;
    case GlobalResearch::TypeEngineer:
      TargetEngineer(_pos);
      break;
    case GlobalResearch::TypeOfficer:
      TargetOfficer(_pos);
      break;
    case GlobalResearch::TypeArmour:
      TargetArmour(_pos);
      break;
  }
}

void Task::TargetSquad(const LegacyVector3& _pos)
{
  int teamId = g_app->m_globalWorld->m_myTeamId;

  int numEntities = 2 + g_app->m_globalWorld->m_research->CurrentLevel(GlobalResearch::TypeSquad);

  int unitId;
  g_app->m_location->m_teams[teamId].NewUnit(EntityType::TypeSquadie, numEntities, &unitId, _pos);
  g_app->m_location->SpawnEntities(_pos, teamId, unitId, EntityType::TypeSquadie, numEntities, g_zeroVector, 10);

  g_app->m_location->m_teams[teamId].SelectUnit(unitId, -1, -1);
  m_objId.Set(teamId, unitId, -1, -1);

  m_state = StateRunning;

  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);

  int trackEntity = g_prefsManager->GetInt(OTHER_AUTOMATICCAM, 0);
  if (trackEntity == 0)
  {
    // work out if player is using control pad
    if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
      trackEntity = 2;
  }

  if (trackEntity == 2)
    g_app->m_camera->RequestEntityTrackMode(m_objId);
}

void Task::TargetEngineer(const LegacyVector3& _pos)
{
  int teamId = g_app->m_globalWorld->m_myTeamId;

  LegacyVector3 pos = _pos;
  pos.y += 10.0f;
  m_objId = g_app->m_location->SpawnEntities(pos, teamId, -1, EntityType::TypeEngineer, 1, g_zeroVector, 0);
  g_app->m_location->m_teams[teamId].SelectUnit(-1, m_objId.GetIndex(), -1);

  m_state = StateRunning;
  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);
}

void Task::TargetArmour(const LegacyVector3& _pos)
{
#ifndef DEMOBUILD
  int teamId = g_app->m_globalWorld->m_myTeamId;

  m_objId = g_app->m_location->SpawnEntities(_pos, teamId, -1, EntityType::TypeArmour, 1, g_zeroVector, 0);
  g_app->m_location->m_teams[teamId].SelectUnit(-1, m_objId.GetIndex(), -1);

  m_state = StateRunning;

  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);
#endif
}

WorldObjectId Task::Promote(WorldObjectId _id)
{
  int teamId = g_app->m_globalWorld->m_myTeamId;

  Entity* entity = g_app->m_location->GetEntity(_id);
  DEBUG_ASSERT(entity);

  //
  // Spawn an Officer

  WorldObjectId spawnedId = g_app->m_location->SpawnEntities(entity->m_pos, teamId, -1, EntityType::TypeOfficer, 1, entity->m_vel, 0);
  auto officer = static_cast<Officer*>(g_app->m_location->GetEntity(spawnedId));
  DEBUG_ASSERT(officer);

  //
  // Particle effect

  int numFlashes = 5 + darwiniaRandom() % 5;
  for (int i = 0; i < numFlashes; ++i)
  {
    LegacyVector3 vel(sfrand(5.0f), frand(15.0f), sfrand(5.0f));
    g_app->m_particleSystem->CreateParticle(entity->m_pos, vel, Particle::TypeControlFlash);
  }

  auto darwinian = static_cast<Darwinian*>(entity);
  darwinian->m_promoted = true;

  return spawnedId;
}

WorldObjectId Task::Demote(WorldObjectId _id)
{
  // Make demoted officers return to green
  //int teamId = g_app->m_globalWorld->m_myTeamId;
  int teamId = 0;

  Entity* entity = g_app->m_location->GetEntity(_id);
  DEBUG_ASSERT(entity);

  //
  // Spawn a Darwinian

  WorldObjectId spawnedId = g_app->m_location->SpawnEntities(entity->m_pos, teamId, -1, EntityType::TypeDarwinian, 1, entity->m_vel, 0);

  //
  // Particle effect

  int numFlashes = 5 + darwiniaRandom() % 5;
  for (int i = 0; i < numFlashes; ++i)
  {
    LegacyVector3 vel(sfrand(5.0f), frand(15.0f), sfrand(5.0f));
    g_app->m_particleSystem->CreateParticle(entity->m_pos, vel, Particle::TypeControlFlash);
  }

  return spawnedId;
}

WorldObjectId Task::FindDarwinian(const LegacyVector3& _pos)
{
  int teamId = g_app->m_globalWorld->m_myTeamId;

  int numFound;
  WorldObjectId* ids = g_app->m_location->m_entityGrid->GetFriends(_pos.x, _pos.z, 10.0f, &numFound, teamId);
  WorldObjectId nearestId;
  float nearest = 99999.9f;

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = ids[i];
    Entity* entity = g_app->m_location->GetEntity(id);
    if (entity && entity->m_entityType == EntityType::TypeDarwinian)
    {
      float distance = (entity->m_pos - _pos).MagSquared();
      if (distance < nearest)
      {
        nearestId = id;
        nearest = distance;
      }
    }
  }

  return nearestId;
}

void Task::TargetOfficer(const LegacyVector3& _pos)
{
  //
  // We will not upgrade people if we're controlling something right now

  Team* myTeam = g_app->m_location->GetMyTeam();
  if (myTeam->m_currentUnitId != -1 || myTeam->m_currentEntityId != -1 || myTeam->m_currentBuildingId != -1)
    return;

  //
  // Find the nearest friendly Darwinian to upgrade to an Officer
  // If we found someone, promote them
  // Then shutdown this task
  // Then select them

  WorldObjectId nearestId = FindDarwinian(_pos);

  if (nearestId.IsValid())
  {
    WorldObjectId id = Promote(nearestId);
    g_app->m_taskManager->TerminateTask(m_id);
    g_app->m_location->m_teams[id.GetTeamId()].SelectUnit(id.GetUnitId(), id.GetIndex(), -1);
    g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageSuccess, GlobalResearch::TypeOfficer, 2.5f);

    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);
  }
}

bool Task::Advance()
{
  if (m_state == StateRunning)
  {
    switch (m_type)
    {
      case GlobalResearch::TypeSquad:
      case GlobalResearch::TypeController:
      {
        Unit* unit = g_app->m_location->GetUnit(m_objId);
        if (!unit || unit->NumAliveEntities() == 0)
        {
          if (g_app->m_taskManager->m_currentTaskId == m_id)
            g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageShutdown, m_type, 3.0f);
          return true;
        }
        break;
      }

      case GlobalResearch::TypeEngineer:
      case GlobalResearch::TypeArmour:
      {
        Entity* entity = g_app->m_location->GetEntity(m_objId);
        if (!entity || entity->m_dead)
        {
          if (g_app->m_taskManager->m_currentTaskId == m_id)
            g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageShutdown, m_type, 3.0f);
          return true;
        }
        break;
      }

      case GlobalResearch::TypeOfficer:
      {
        m_state = StateStarted;
        break;
      }
    }
  }

  return (m_state == StateStopping);
}

void Task::SwitchTo()
{
  if (g_app->m_camera->IsInMode(Camera::ModeRadarAim) || g_app->m_camera->IsInMode(Camera::ModeTurretAim))
    g_app->m_camera->RequestMode(Camera::ModeFreeMovement);

  int teamId = g_app->m_globalWorld->m_myTeamId;

  switch (m_type)
  {
    case GlobalResearch::TypeSquad:
    {
      g_app->m_location->m_teams[teamId].SelectUnit(m_objId.GetUnitId(), -1, -1);
      break;
    }

    case GlobalResearch::TypeEngineer:
    case GlobalResearch::TypeArmour:
    {
      g_app->m_location->m_teams[teamId].SelectUnit(-1, m_objId.GetIndex(), -1);
      break;
    }

    case GlobalResearch::TypeController:
    {
      for (int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i)
      {
        Task* task = g_app->m_taskManager->m_tasks.GetData(i);
        if (task->m_type == GlobalResearch::TypeSquad && task->m_objId == m_objId)
        {
          g_app->m_taskManager->SelectTask(task->m_id);
          break;
        }
      }
      break;
    }

    case GlobalResearch::TypeOfficer:
    {
      g_app->m_location->m_teams[teamId].SelectUnit(-1, -1, -1);
      break;
    }
  }
}

void Task::Stop()
{
  switch (m_type)
  {
    case GlobalResearch::TypeSquad:
    {
      if (Unit* unit = g_app->m_location->GetUnit(m_objId))
      {
        for (int i = 0; i < unit->m_entities.Size(); ++i)
        {
          if (unit->m_entities.ValidIndex(i))
          {
            Entity* entity = unit->m_entities[i];
            entity->ChangeHealth(-1000);
          }
        }
      }
      break;
    }

    case GlobalResearch::TypeEngineer:
    case GlobalResearch::TypeArmour:
    {
      if (auto entity = g_app->m_location->GetEntity(m_objId))
      {
        entity->ChangeHealth(-1000);
      }
      break;
    }
  }

  m_state = StateStopping;
}

TaskManager::TaskManager()
  : m_nextTaskId(0),
    m_currentTaskId(-1),
    m_verifyTargetting(true) {}

bool TaskManager::RunTask(Task* _task)
{
  if (CapacityUsed() < Capacity())
  {
    _task->m_id = m_nextTaskId;
    ++m_nextTaskId;
    m_tasks.PutDataAtEnd(_task);
    _task->Start();
    m_currentTaskId = _task->m_id;

    return true;
  }
  g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageFailure, -1, 2.5f);

  return false;
}

bool TaskManager::RunTask(int _type)
{
  switch (_type)
  {
    case GlobalResearch::TypeSquad:
    case GlobalResearch::TypeEngineer:
    case GlobalResearch::TypeOfficer:
#ifndef DEMOBUILD
    case GlobalResearch::TypeArmour:
#endif
    {
      auto task = NEW Task();
      task->m_type = _type;
      bool success = RunTask(task);
      if (success)
      {
        int teamId = g_app->m_globalWorld->m_myTeamId;
        g_app->m_location->m_teams[teamId].SelectUnit(-1, -1, -1);
      }
      return success;
    }

    case GlobalResearch::TypeController:
    {
      Task* task = GetCurrentTask();
      if (task && task->m_type == GlobalResearch::TypeSquad)
      {
        Unit* unit = g_app->m_location->GetUnit(task->m_objId);
        if (unit && unit->m_troopType == EntityType::TypeSquadie)
        {
          auto squad = static_cast<InsertionSquad*>(unit);
          squad->SetWeaponType(_type);

          if (!GetTask(squad->m_controllerId))
          {
            auto controller = NEW Task();
            controller->m_type = _type;
            controller->m_objId = WorldObjectId(squad->m_teamId, squad->m_unitId, -1, -1);
            controller->m_route = NEW Route(-1);
            controller->m_route->AddWayPoint(squad->m_centerPos);
            bool success = RunTask(controller);
            if (success)
            {
              squad->m_controllerId = controller->m_id;
              SelectTask(task->m_id);
              g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageSuccess, _type, 2.5f);
            }
            return success;
          }

          return true;
        }
      }

      g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageFailure, -1, 2.5f);
      return false;
    }

    case GlobalResearch::TypeGrenade:
    case GlobalResearch::TypeRocket:
    case GlobalResearch::TypeAirStrike:
    {
      Task* task = GetCurrentTask();
      if (task && task->m_type == GlobalResearch::TypeSquad)
      {
        Unit* unit = g_app->m_location->GetUnit(task->m_objId);
        if (unit && unit->m_troopType == EntityType::TypeSquadie)
        {
          auto squad = static_cast<InsertionSquad*>(unit);
          squad->SetWeaponType(_type);
          g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageSuccess, _type, 2.5f);
          return true;
        }
      }

      g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageFailure, -1, 2.5f);
      return false;
    }
  }

  return false;
}

bool TaskManager::RegisterTask(Task* _task)
{
  if (CapacityUsed() < Capacity())
  {
    _task->m_id = m_nextTaskId;
    ++m_nextTaskId;
    m_tasks.PutDataAtEnd(_task);
    return true;
  }

  return false;
}

bool TaskManager::TerminateTask(int _id)
{
  for (int i = 0; i < m_tasks.Size(); ++i)
  {
    Task* task = m_tasks[i];
    if (task->m_id == _id)
    {
      g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageShutdown, task->m_type, 3.0f);
      m_tasks.RemoveData(i);
      task->Stop();
      delete task;

      if (m_currentTaskId == _id)
        m_currentTaskId = -1;

      return true;
    }
  }

  return false;
}

int TaskManager::Capacity()
{
  int taskManagerResearch = g_app->m_globalWorld->m_research->CurrentLevel(GlobalResearch::TypeTaskManager);
  int capacity = 1 + taskManagerResearch;
  return capacity;
}

int TaskManager::CapacityUsed() { return m_tasks.Size(); }

void TaskManager::AdvanceTasks()
{
  //
  // Are we currently placing a task?

  Task* currentTask = GetCurrentTask();
  if (currentTask && currentTask->m_state == Task::StateStarted)
    g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageSuccess, currentTask->m_type, 2.5f);

  //
  // Advance all other tasks

  for (int i = 0; i < m_tasks.Size(); ++i)
  {
    Task* task = m_tasks[i];
    bool amIDone = task->Advance();
    if (amIDone)
    {
      if (m_currentTaskId == task->m_id)
      {
        m_currentTaskId = -1;
        int teamId = g_app->m_globalWorld->m_myTeamId;
        g_app->m_location->m_teams[teamId].SelectUnit(-1, -1, -1);
      }

      m_tasks.RemoveData(i);
      --i;
      delete task;
    }
  }
}

void TaskManager::StopAllTasks()
{
  m_tasks.EmptyAndDelete();
  m_currentTaskId = -1;
}

void TaskManager::Advance()
{
  AdvanceTasks();

  g_app->m_globalWorld->m_research->AdvanceResearch();
}

void TaskManager::SelectTask(int _id)
{
  m_currentTaskId = _id;
  if (m_currentTaskId != -1)
  {
    int currentIndex = -1;
    for (int i = 0; i < m_tasks.Size(); ++i)
    {
      Task* task = m_tasks[i];
      if (task->m_id == m_currentTaskId)
      {
        currentIndex = i;
        break;
      }
    }

    ASSERT_TEXT(currentIndex != -1, "Error in TaskManager::SelectTask. Tried to select a task that doesn't exist.");

    Task* task = m_tasks[currentIndex];
    //m_tasks.RemoveData(currentIndex);
    //m_tasks.PutDataAtStart( task );
    task->SwitchTo();

    g_app->m_gameCursor->BoostSelectionArrows(2.0f);
  }
}

void TaskManager::SelectTask(WorldObjectId _id)
{
  for (int i = 0; i < m_tasks.Size(); ++i)
  {
    Task* task = m_tasks[i];
    if (task->m_objId.GetTeamId() == _id.GetTeamId() && task->m_objId.GetUnitId() == _id.GetUnitId() && task->m_objId.GetIndex() ==
      _id.GetIndex())
    {
      SelectTask(task->m_id);
      break;
    }
  }
}

Task* TaskManager::GetCurrentTask() { return GetTask(m_currentTaskId); }

Task* TaskManager::GetTask(int _id)
{
  for (int i = 0; i < m_tasks.Size(); ++i)
  {
    Task* task = m_tasks[i];
    if (task->m_id == _id)
      return task;
  }

  return nullptr;
}

Task* TaskManager::GetTask(WorldObjectId _id)
{
  for (int i = 0; i < m_tasks.Size(); ++i)
  {
    Task* task = m_tasks[i];
    if (task->m_objId == _id)
      return task;
  }

  return nullptr;
}

bool TaskManager::TargetTask(int _id, const LegacyVector3& _pos)
{
  if (IsValidTargetArea(_id, _pos))
  {
    Task* task = GetTask(_id);
    task->Target(_pos);
    return true;
  }

  return false;
}

bool TaskManager::IsValidTargetArea(int _id, const LegacyVector3& _pos)
{
  Task* task = g_app->m_taskManager->GetTask(_id);
  if (!task)
    return false;

  if (!g_app->m_location || !g_app->m_location->m_landscape.IsInLandscape(_pos))
    return false;

  if (m_verifyTargetting)
  {
    if (task->m_type == GlobalResearch::TypeOfficer)
    {
      int numFound;
      WorldObjectId* ids = g_app->m_location->m_entityGrid->GetFriends(_pos.x, _pos.z, 10.0f, &numFound,
                                                                       g_app->m_globalWorld->m_myTeamId);
      bool foundDarwinian = false;
      for (int i = 0; i < numFound; ++i)
      {
        WorldObjectId id = ids[i];
        Entity* ent = g_app->m_location->GetEntity(id);
        if (ent && ent->m_entityType == EntityType::TypeDarwinian)
        {
          foundDarwinian = true;
          break;
        }
      }
      return foundDarwinian;
    }
    LList<TaskTargetArea>* targetAreas = GetTargetArea(_id);
    bool success = false;

    for (int i = 0; i < targetAreas->Size(); ++i)
    {
      TaskTargetArea* targetArea = targetAreas->GetPointer(i);
      if ((_pos - targetArea->m_center).Mag() <= targetArea->m_radius)
      {
        success = true;
        break;
      }
    }

    delete targetAreas;
    return success;
  }
  return true;
}

LList<TaskTargetArea>* TaskManager::GetTargetArea(int _id)
{
  auto result = NEW LList<TaskTargetArea>();

  Task* task = GetTask(_id);
  if (task)
  {
    switch (task->m_type)
    {
      case GlobalResearch::TypeArmour:
      {
        for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
        {
          if (g_app->m_location->m_buildings.ValidIndex(i))
          {
            Building* building = g_app->m_location->m_buildings[i];
            if (building && building->m_buildingType == BuildingType::TypeTrunkPort && static_cast<TrunkPort*>(building)->m_openTimer > 0.0f)
            {
              TaskTargetArea tta;
              tta.m_center = building->m_pos;
              tta.m_radius = 120.0f;
              tta.m_stationary = true;
              result->PutData(tta);
            }
          }
        }
        break;
      }

      case GlobalResearch::TypeEngineer:
      {
        Team* team = g_app->m_location->GetMyTeam();
        for (int i = 0; i < team->m_units.Size(); ++i)
        {
          if (team->m_units.ValidIndex(i))
          {
            Unit* unit = team->m_units[i];
            if (unit->m_troopType == EntityType::TypeSquadie)
            {
              TaskTargetArea tta;
              tta.m_center = unit->m_centerPos;
              tta.m_radius = 100.0f;
              tta.m_stationary = false;
              result->PutData(tta);
            }
          }
        }
        //break;                // DELIBERATE FALL THROUGH
      }

      case GlobalResearch::TypeSquad:
        for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
        {
          if (g_app->m_location->m_buildings.ValidIndex(i))
          {
            Building* building = g_app->m_location->m_buildings[i];
            if (building && building->m_buildingType == BuildingType::TypeControlTower && building->m_id.GetTeamId() == g_app->m_location->
              GetMyTeam()->m_teamId)
            {
              TaskTargetArea tta;
              tta.m_center = building->m_pos;
              tta.m_radius = 75.0f;
              tta.m_stationary = true;
              result->PutData(tta);
            }
          }
        }
        break;

      case GlobalResearch::TypeOfficer:
      {
        TaskTargetArea tta;
        tta.m_center.Set(0, 0, 0);
        tta.m_radius = 99999.9f;
        result->PutData(tta);
        break;
      }
    }
  }

  return result;
}
