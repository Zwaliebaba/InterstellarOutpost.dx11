#include "pch.h"
#include "cheat_window.h"
#include "GameApp.h"
#include "camera.h"
#include "debugmenu.h"
#include "global_world.h"
#include "globals.h"
#include "location.h"
#include "math_utils.h"
#include "Renderer.h"
#include "resource.h"
#include "taskmanager.h"
#include "team.h"
#include "unit.h"

#ifdef CHEATMENU_ENABLED

class KillAllEnemiesButton : public GuiButton
{
  void MouseUp() override
  {
    if (g_app->m_location)
    {
      int myTeamId = g_app->m_globalWorld->m_myTeamId;
      for (int t = 0; t < NUM_TEAMS; ++t)
      {
        if (!g_app->m_location->IsFriend(myTeamId, t))
        {
          Team* team = &g_app->m_location->m_teams[t];

          // Kill all UNITS
          for (int u = 0; u < team->m_units.Size(); ++u)
          {
            if (team->m_units.ValidIndex(u))
            {
              Unit* unit = team->m_units[u];
              for (int e = 0; e < unit->m_entities.Size(); ++e)
              {
                if (unit->m_entities.ValidIndex(e))
                {
                  Entity* entity = unit->m_entities[e];
                  entity->ChangeHealth(entity->m_stats[Entity::StatHealth] * -2.0f);
                }
              }
            }
          }

          // Kill all ENTITIES
          for (int o = 0; o < team->m_others.Size(); ++o)
          {
            if (team->m_others.ValidIndex(o))
            {
              Entity* entity = team->m_others[o];
              entity->ChangeHealth(entity->m_stats[Entity::StatHealth] * -2.0f);
            }
          }
        }
      }

      for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
      {
        if (g_app->m_location->m_buildings.ValidIndex(i))
        {
          Building* building = g_app->m_location->m_buildings[i];
          if (building->m_buildingType == BuildingType::TypeAntHill || building->m_buildingType == BuildingType::TypeTriffid)
            building->Damage(-999);
        }
      }
    }
  }
};

class SpawnDarwiniansButton : public GuiButton
{
  public:
    int m_teamId;

    void MouseUp() override
    {
      if (g_app->m_location)
      {
        LegacyVector3 rayStart;
        LegacyVector3 rayDir;
        g_app->m_camera->GetClickRay(ClientEngine::OutputSize().Width / 2, ClientEngine::OutputSize().Height / 2, &rayStart, &rayDir);
        LegacyVector3 _pos;
        g_app->m_location->m_landscape.RayHit(rayStart, rayDir, &_pos);

        g_app->m_location->SpawnEntities(_pos, m_teamId, -1, EntityType::TypeDarwinian, 20, g_zeroVector, 30);
      }
    }
};

class SpawnTankButton : public GuiButton
{
  void MouseUp() override
  {
    if (g_app->m_location)
    {
      LegacyVector3 rayStart;
      LegacyVector3 rayDir;
      g_app->m_camera->GetClickRay(ClientEngine::OutputSize().Width / 2, ClientEngine::OutputSize().Height / 2, &rayStart, &rayDir);
      LegacyVector3 _pos;
      g_app->m_location->m_landscape.RayHit(rayStart, rayDir, &_pos);

      g_app->m_location->SpawnEntities(_pos, 2, -1, EntityType::TypeArmour, 1, g_zeroVector, 0);
    }
  }
};

class SpawnViriiButton : public GuiButton
{
  void MouseUp() override
  {
    if (g_app->m_location)
    {
      LegacyVector3 rayStart;
      LegacyVector3 rayDir;
      g_app->m_camera->GetClickRay(ClientEngine::OutputSize().Width / 2, ClientEngine::OutputSize().Height / 2, &rayStart, &rayDir);
      LegacyVector3 _pos;
      g_app->m_location->m_landscape.RayHit(rayStart, rayDir, &_pos);

      g_app->m_location->SpawnEntities(_pos, 1, -1, EntityType::TypeVirii, 20, g_zeroVector, 0, 1000.0f);
    }
  }
};

class SpawnSpiritButton : public GuiButton
{
  void MouseUp() override
  {
    if (g_app->m_location)
    {
      LegacyVector3 rayStart;
      LegacyVector3 rayDir;
      g_app->m_camera->GetClickRay(ClientEngine::OutputSize().Width / 2, ClientEngine::OutputSize().Height / 2, &rayStart, &rayDir);
      LegacyVector3 _pos;
      g_app->m_location->m_landscape.RayHit(rayStart, rayDir, &_pos);

      for (int i = 0; i < 10; ++i)
      {
        LegacyVector3 spiritPos = _pos + LegacyVector3(syncsfrand(20.0f), 0.0f, syncsfrand(20.0f));
        g_app->m_location->SpawnSpirit(spiritPos, g_zeroVector, 2, WorldObjectId());
      }
    }
  }
};

class AllowArbitraryPlacementButton : public GuiButton
{
  void MouseUp() override { g_app->m_taskManager->m_verifyTargetting = false; }
};

class EnableGeneratorAndMineButton : public GuiButton
{
  void MouseUp() override
  {
    int generatorLocationId = g_app->m_globalWorld->GetLocationId("generator");
    int mineLocationId = g_app->m_globalWorld->GetLocationId("mine");

    for (int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i)
    {
      if (g_app->m_globalWorld->m_buildings.ValidIndex(i))
      {
        GlobalBuilding* gb = g_app->m_globalWorld->m_buildings[i];
        if (gb && gb->m_locationId == generatorLocationId && gb->m_buildingType == BuildingType::TypeGenerator)
          gb->m_online = true;

        if (gb && gb->m_locationId == mineLocationId && gb->m_buildingType == BuildingType::TypeRefinery)
          gb->m_online = true;
      }
    }

    g_app->m_globalWorld->EvaluateEvents();
  }
};

class EnableReceiverAndBufferButton : public GuiButton
{
  void MouseUp() override
  {
    int receiverLocationId = g_app->m_globalWorld->GetLocationId("receiver");
    int bufferLocationId = g_app->m_globalWorld->GetLocationId("pattern_buffer");

    for (int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i)
    {
      if (g_app->m_globalWorld->m_buildings.ValidIndex(i))
      {
        GlobalBuilding* gb = g_app->m_globalWorld->m_buildings[i];
        if (gb && gb->m_locationId == receiverLocationId && gb->m_buildingType == BuildingType::TypeSpiritProcessor)
          gb->m_online = true;

        if (gb && gb->m_locationId == bufferLocationId && gb->m_buildingType == BuildingType::TypeBlueprintStore)
          gb->m_online = true;
      }
    }

    g_app->m_globalWorld->EvaluateEvents();
  }
};

class OpenAllLocationsButton : public GuiButton
{
  void MouseUp() override
  {
    for (int i = 0; i < g_app->m_globalWorld->m_locations.Size(); ++i)
    {
      GlobalLocation* loc = g_app->m_globalWorld->m_locations[i];
      loc->m_available = true;
    }
  }
};

class ClearResourcesButton : public GuiButton
{
  void MouseUp() override
  {
    Resource::FlushOpenGlState();
    Resource::RegenerateOpenGlState();
  }
};

class GiveAllResearchButton : public GuiButton
{
  void MouseUp() override
  {
    for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
    {
      g_app->m_globalWorld->m_research->AddResearch(i);
      g_app->m_globalWorld->m_research->EvaluateLevel(i);
    }
  }
};

class SpawnPortsButton : public GuiButton
{
  void MouseUp() override
  {
    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        for (int p = 0; p < building->GetNumPorts(); ++p)
        {
          LegacyVector3 portPos, portFront;
          building->GetPortPosition(p, portPos, portFront);

          g_app->m_location->SpawnEntities(portPos, 0, -1, EntityType::TypeDarwinian, 3, g_zeroVector, 30.0f);
        }
      }
    }
  }
};

#ifdef PROFILER_ENABLED
class ProfilerCreateButton : public GuiButton
{
  void MouseUp() override { DebugKeyBindings::ProfileButton(); }
};
#endif // PROFILER_ENABLED

void CheatWindow::Create()
{
  GuiWindow::Create();

  int y = 25;

  auto killAllEnemies = NEW KillAllEnemiesButton();
  killAllEnemies->SetProperties("Kill All Enemies", 10, y, m_w - 20);
  RegisterButton(killAllEnemies);

  auto spawnDarwiniansGreen = NEW SpawnDarwiniansButton();
  spawnDarwiniansGreen->SetProperties("Spawn Green", 10, y += 20, (m_w - 25) / 2);
  spawnDarwiniansGreen->m_teamId = 0;
  RegisterButton(spawnDarwiniansGreen);

  auto spawnDarwiniansRed = NEW SpawnDarwiniansButton();
  spawnDarwiniansRed->SetProperties("Spawn Red", spawnDarwiniansGreen->m_x + spawnDarwiniansGreen->m_w + 5, y, (m_w - 25) / 2);
  spawnDarwiniansRed->m_teamId = 1;
  RegisterButton(spawnDarwiniansRed);

  auto spawnTankButton = NEW SpawnTankButton();
  spawnTankButton->SetProperties("Spawn Armour", 10, y += 20, m_w - 20);
  RegisterButton(spawnTankButton);

  auto spawnVirii = NEW SpawnViriiButton();
  spawnVirii->SetProperties("Spawn Virii", 10, y += 20, m_w - 20);
  RegisterButton(spawnVirii);

  auto spawnSpirits = NEW SpawnSpiritButton();
  spawnSpirits->SetProperties("Spawn Spirits", 10, y += 20, m_w - 20);
  RegisterButton(spawnSpirits);

  auto allowPlacement = NEW AllowArbitraryPlacementButton();
  allowPlacement->SetProperties("Allow Arbitrary Placement", 10, y += 20, m_w - 20);
  RegisterButton(allowPlacement);

  auto enable = NEW EnableGeneratorAndMineButton();
  enable->SetProperties("Enable Generator and Mine", 10, y += 20, m_w - 20);
  RegisterButton(enable);

  auto receiver = NEW EnableReceiverAndBufferButton();
  receiver->SetProperties("Enable Receiver and Buffer", 10, y += 20, m_w - 20);
  RegisterButton(receiver);

  auto openAllLocations = NEW OpenAllLocationsButton();
  openAllLocations->SetProperties("Open All Locations", 10, y += 20, m_w - 20);
  RegisterButton(openAllLocations);

  auto allResearch = NEW GiveAllResearchButton();
  allResearch->SetProperties("Give all research", 10, y += 20, m_w - 20);
  RegisterButton(allResearch);

  auto clearResources = NEW ClearResourcesButton();
  clearResources->SetProperties("Clear Resources", 10, y += 20, m_w - 20);
  RegisterButton(clearResources);

  auto spawnPorts = NEW SpawnPortsButton();
  spawnPorts->SetProperties("Spawn Ports", 10, y += 20, m_w - 20);
  RegisterButton(spawnPorts);

  y += 20;

#ifdef PROFILER_ENABLED
  auto profiler = NEW ProfilerCreateButton();
  profiler->SetProperties("Profiler", 10, y += 20, m_w - 20);
  RegisterButton(profiler);
#endif // PROFILER_ENABLED
}

#else   // CHEATMENU_ENABLED

void CheatWindow::Create()
{
}

#endif  // CHEATMENU_ENABLED

CheatWindow::CheatWindow(const char* _name)
  : GuiWindow(_name) {}
