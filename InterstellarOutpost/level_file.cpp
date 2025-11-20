#include "pch.h"
#include "level_file.h"
#include "GameApp.h"
#include "RoutingSystem.h"
#include "ai.h"
#include "anthill.h"
#include "global_world.h"
#include "location.h"
#include "officer.h"
#include "researchitem.h"
#include "resource.h"
#include "spawnpoint.h"
#include "text_stream_readers.h"
#include "triffid.h"

constexpr std::string_view TRANSITION_MODE_NAMES[] = {"Move", "Cut"};

TransitionMode CamAnimNode::GetTransitModeId(const char* _word)
{
  for (const auto i : RangeTransitionMode())
  {
    if (_word == TRANSITION_MODE_NAMES[I(i)])
      return i;
  }
  return TransitionMode::Unknown;
}

std::string CamAnimNode::GetTransitModeName(TransitionMode _modeId)
{
  return std::string(TRANSITION_MODE_NAMES[I(_modeId)]);
}

void LevelFile::ParseMissionFile(std::string_view _filename)
{
  TextReader* in = nullptr;
  char fullFilename[256];

  sprintf(fullFilename, "levels\\%s", _filename.data());
  in = Resource::GetTextReader(fullFilename);

  ASSERT_TEXT(in && in->IsOpen(), "Invalid level specified");

  while (in->ReadLine())
  {
    if (!in->TokenAvailable())
      continue;
    char* word = in->GetNextToken();

    if (_stricmp("Landscape_StartDefinition", word) == 0 || _stricmp("LandscapeTiles_StartDefinition", word) == 0 ||
      _stricmp("LandFlattenAreas_StartDefinition", word) == 0 || _stricmp("Lights_StartDefinition", word) == 0)
      DEBUG_ASSERT(0);
    if (_stricmp("CameraMounts_StartDefinition", word) == 0)
      ParseCameraMounts(in);
    else if (_stricmp("CameraAnimations_StartDefinition", word) == 0)
      ParseCameraAnims(in);
    else if (_stricmp("Buildings_StartDefinition", word) == 0)
      ParseBuildings(in, true);
    else if (_stricmp("InstantUnits_StartDefinition", word) == 0)
      ParseInstantUnits(in);
    else if (_stricmp("Routes_StartDefinition", word) == 0)
      ParseRoutes(in);
    else if (_stricmp("PrimaryObjectives_StartDefinition", word) == 0)
      ParsePrimaryObjectives(in);
    else if (_stricmp("RunningPrograms_StartDefinition", word) == 0)
      ParseRunningPrograms(in);
    else if (_stricmp("Difficulty_StartDefinition", word) == 0)
      ParseDifficulty(in);
    else
    {
      // Looks like a damaged level file
      DEBUG_ASSERT(0);
    }
  }

  delete in;
}

void LevelFile::ParseMapFile(std::string_view _levelFilename)
{
  std::string fullFilename = "levels\\" + std::string(_levelFilename);
  TextReader* in = Resource::GetTextReader(fullFilename);
  ASSERT_TEXT(in && in->IsOpen(), "Invalid map file specified ({})", _levelFilename);

  while (in->ReadLine())
  {
    if (!in->TokenAvailable())
      continue;
    char* word = in->GetNextToken();

    if (_stricmp("landscape_startDefinition", word) == 0)
      ParseLandscapeData(in);
    else if (_stricmp("landscapeTiles_startDefinition", word) == 0)
      ParseLandscapeTiles(in);
    else if (_stricmp("landFlattenAreas_startDefinition", word) == 0)
      ParseLandFlattenAreas(in);
    else if (_stricmp("Buildings_StartDefinition", word) == 0)
      ParseBuildings(in, false);
    else if (_stricmp("Lights_StartDefinition", word) == 0)
      ParseLights(in);
    else
      DEBUG_ASSERT(0);
  }

  delete in;
}

void LevelFile::ParseCameraMounts(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("CameraMounts_EndDefinition", word) == 0)
      return;

    auto cmnt = NEW CameraMount();

    // Read name
    cmnt->m_name = word;

    // Read pos
    word = _in->GetNextToken();
    cmnt->m_pos.x = atof(word);
    word = _in->GetNextToken();
    cmnt->m_pos.y = atof(word);
    word = _in->GetNextToken();
    cmnt->m_pos.z = atof(word);

    // Read front
    word = _in->GetNextToken();
    cmnt->m_front.x = atof(word);
    word = _in->GetNextToken();
    cmnt->m_front.y = atof(word);
    word = _in->GetNextToken();
    cmnt->m_front.z = atof(word);
    cmnt->m_front.Normalize();

    // Read up
    word = _in->GetNextToken();
    cmnt->m_up.x = atof(word);
    word = _in->GetNextToken();
    cmnt->m_up.y = atof(word);
    word = _in->GetNextToken();
    cmnt->m_up.z = atof(word);
    cmnt->m_up.Normalize();

    m_cameraMounts.PutData(cmnt);
  }
}

void LevelFile::ParseCameraAnims(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("CameraAnimations_EndDefinition", word) == 0)
      return;

    auto anim = NEW CameraAnimation();
    anim->m_name = word;

    while (_in->ReadLine())
    {
      if (!_in->TokenAvailable())
        continue;
      word = _in->GetNextToken();
      if (_stricmp(word, "End") == 0)
        break;

      CamAnimNode node;

      // Read camera mode
      node.m_transitionMode = CamAnimNode::GetTransitModeId(word);
      ASSERT_TEXT(node.m_transitionMode != TransitionMode::Unknown,"Bad camera animation camera mode in level file {}", m_missionFilename);

      // Read mount name
      word = _in->GetNextToken();
      node.m_mountName = StrUpr(word);
      if (node.m_mountName != MAGIC_MOUNT_NAME_START_POS)
      {
        ASSERT_TEXT(GetCameraMount(node.m_mountName), "Bad camera animation mount name in level file {}", m_missionFilename);
      }

      // Read time
      word = _in->GetNextToken();
      node.m_duration = atof(word);
      ASSERT_TEXT(node.m_duration >= 0.0f && node.m_duration < 60.0f, "Bad camera animation transition time in level file {}", m_missionFilename);

      anim->m_nodes.emplace_back(node);
    }

    m_cameraAnimations.PutData(anim);
  }
}

void LevelFile::ParseBuildings(TextReader* _in, bool _dynamic)
{
  float loadDifficultyFactor = 1.0;
  if (m_levelDifficulty < 0)
    loadDifficultyFactor = 1.0f + static_cast<float>(g_app->m_difficultyLevel) / 5.0f;

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("Buildings_EndDefinition", word) == 0)
      return;

    Building* building = Building::CreateBuilding(word);
    if (building)
    {
      building->Read(_in, _dynamic);

      // Make sure that this building's ID isn't already in use
      int uniqueId = building->m_id.GetUniqueId();
      Building* existingBuilding = GetBuilding(uniqueId);
      if (existingBuilding)
      {
        Fatal("{} UniqueId was not unique in {}", Building::GetTypeName(existingBuilding->m_buildingType), _in->GetFilename());
      }

      // Make sure that it's global if it needs to be
      if (building->m_buildingType == BuildingType::TypeTrunkPort || building->m_buildingType == BuildingType::TypeControlTower || building->m_buildingType ==
        BuildingType::TypeRadarDish || building->m_buildingType == BuildingType::TypeIncubator || building->m_buildingType == BuildingType::TypeFenceSwitch)
      {
        ASSERT_TEXT(building->m_isGlobal, "Non-global {} found in {}", Building::GetTypeName(building->m_buildingType), _in->GetFilename());
      }

      // Increase the difficulty by raising the population limits for the opposing forces
      if (building->m_id.GetTeamId() == 1)
      {
        switch (building->m_buildingType)
        {
          case BuildingType::TypeSpawnPopulationLock:
          {
            auto spl = dynamic_cast<SpawnPopulationLock*>(building);
            spl->m_maxPopulation = static_cast<int>(static_cast<float>(spl->m_maxPopulation) * loadDifficultyFactor);
          }
          break;

          case BuildingType::TypeAntHill:
          {
            auto ah = dynamic_cast<AntHill*>(building);
            ah->m_numAntsInside = static_cast<int>(static_cast<float>(ah->m_numAntsInside) * loadDifficultyFactor);
          }
          break;

          case BuildingType::TypeAISpawnPoint:
          {
            auto aisp = dynamic_cast<AISpawnPoint*>(building);
            aisp->m_period = static_cast<int>(static_cast<float>(aisp->m_period) / loadDifficultyFactor);
          }
          break;

          case BuildingType::TypeTriffid:
          {
            auto t = dynamic_cast<Triffid*>(building);
            t->m_reloadTime = t->m_reloadTime / loadDifficultyFactor;
          }
          break;
        }
      }

      m_buildings.PutData(building);
    }
  }
}

void LevelFile::ParseInstantUnits(TextReader* _in)
{
  float loadDifficultyFactor = 1.0;
  if (m_levelDifficulty < 0)
    loadDifficultyFactor = 1.0f + static_cast<float>(g_app->m_difficultyLevel) / 5.0f;

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("InstantUnits_EndDefinition", word) == 0)
      return;

    auto entityType = Entity::GetTypeId(word);
    if (entityType == EntityType::Invalid)
      continue;

    auto iu = NEW InstantUnit();
    int numCopies = 0;
    iu->m_entityType = entityType;

    iu->m_teamId = atoi(_in->GetNextToken());
    iu->m_posX = atof(_in->GetNextToken());
    iu->m_posZ = atof(_in->GetNextToken());
    iu->m_number = atoi(_in->GetNextToken());
    iu->m_inAUnit = atoi(_in->GetNextToken()) ? true : false;
    iu->m_state = atoi(_in->GetNextToken());
    iu->m_spread = atof(_in->GetNextToken());

    if (_in->TokenAvailable())
    {
      iu->m_waypointX = atof(_in->GetNextToken());
      iu->m_waypointZ = atof(_in->GetNextToken());
    }

    if (_in->TokenAvailable())
    {
      iu->m_routeId = atoi(_in->GetNextToken());
      if (_in->TokenAvailable())
        iu->m_routeWaypointId = atoi(_in->GetNextToken());
    }

    // Stop InstantSquaddies bug from crashing Darwinia (for existing bad save files)
    if (iu->m_entityType == EntityType::TypeSquadie && iu->m_inAUnit == 0)
      continue;

    // If the unit is on the red team then make it more of them
    if (iu->m_teamId == 1)
    {
      iu->m_number = static_cast<int>(iu->m_number * loadDifficultyFactor);

      // It doesn't make sense to make overly
      if (iu->m_entityType == EntityType::TypeCentipede && loadDifficultyFactor > 1)
      {
        int maxCentipedeLength = 10 + static_cast<int>(2.0 * loadDifficultyFactor);
        int segmentUtilisation = maxCentipedeLength + 7;
        if (iu->m_number > maxCentipedeLength)
        {
          numCopies = iu->m_number / segmentUtilisation - 1;
          iu->m_number = maxCentipedeLength;
        }
      }
      else
      {
        if (loadDifficultyFactor > 1.0f)
          iu->m_spread *= pow(1.2, g_app->m_difficultyLevel / 5.0);
      }
    }

    m_instantUnits.PutData(iu);

    // Create some additional centipedes if necessary
    for (int i = 0; i < numCopies; i++)
    {
      auto copy = NEW InstantUnit;
      *copy = *iu;
      // Spread them out a bit
      copy->m_posX = iu->m_posX + sfrand(60);
      copy->m_posZ = iu->m_posZ + sfrand(60);
      m_instantUnits.PutData(copy);
    }
  }
}

void LevelFile::ParseLandscapeData(TextReader* _in)
{
  while (_in->ReadLine())
  {
    char* word = _in->GetNextToken();
    char* secondWord = nullptr;

    if (_in->TokenAvailable())
      secondWord = _in->GetNextToken();

    if (_stricmp("cellSize", word) == 0)
      m_landscape.m_cellSize = atof(secondWord);
    else if (_stricmp("worldSizeX", word) == 0)
      m_landscape.m_worldSizeX = atoi(secondWord);
    else if (_stricmp("worldSizeZ", word) == 0)
      m_landscape.m_worldSizeZ = atoi(secondWord);
    else if (_stricmp("outsideHeight", word) == 0)
      m_landscape.m_outsideHeight = atof(secondWord);
    else if (_stricmp("landColorFile", word) == 0)
      m_landscapeColorFilename = secondWord;
    else if (_stricmp("landscape_endDefinition", word) == 0)
      return;
    else
      DebugTrace("Unknown landscape definition in level file: {}\n", word);
  }
}

void LevelFile::ParseLandscapeTiles(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp(word, "landscapeTiles_endDefinition") == 0)
      return;

    auto def = NEW LandscapeTile();
    m_landscape.m_tiles.PutDataAtEnd(def);

    def->m_posX = atoi(word);

    word = _in->GetNextToken();
    def->m_posY = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_posZ = atoi(word);

    word = _in->GetNextToken();
    def->m_size = atoi(word);

    word = _in->GetNextToken();
    def->m_fractalDimension = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_heightScale = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_desiredHeight = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_generationMethod = static_cast<float>(atoi(word));

    word = _in->GetNextToken();
    def->m_randomSeed = atoi(word);

    word = _in->GetNextToken();
    def->m_lowlandSmoothingFactor = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    int guidePower = atoi(word);
    def->GuideGridSetPower(guidePower);

    if (def->m_guideGridPower > 0)
    {
      word = _in->GetNextToken();
      def->GuideGridFromString(word);
    }
  }
}

void LevelFile::ParseLandFlattenAreas(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("landFlattenAreas_endDefinition", word) == 0)
      return;

    auto def = NEW LandscapeFlattenArea();
    m_landscape.m_flattenAreas.PutDataAtEnd(def);

    def->m_center.x = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_center.y = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_center.z = static_cast<float>(atof(word));

    word = _in->GetNextToken();
    def->m_size = static_cast<float>(atof(word));
  }
}

void LevelFile::ParseLights(TextReader* _in)
{
  bool ignoreLights = false;

  if (m_lights.Size() > 0)
  {
    // This function is called first when parsing the level file and
    // secondly when parsing the map file. We only get here if the
    // level file specified some lights. In which case, these lights
    // are to be used in preference to the map lights. So we need
    // to ignore all the lights we encounter.
    ignoreLights = true;
  }

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("Lights_EndDefinition", word) == 0)
      return;

    if (ignoreLights)
      continue;

    auto light = NEW Light;
    m_lights.PutData(light);

    light->m_front[0] = atof(word);

    word = _in->GetNextToken();
    light->m_front[1] = atof(word);

    word = _in->GetNextToken();
    light->m_front[2] = atof(word);
    light->m_front[3] = 0.0f;	// Set to be an infinitely distant light
    light->Normalize();

    word = _in->GetNextToken();
    light->m_colour[0] = atof(word);

    word = _in->GetNextToken();
    light->m_colour[1] = atof(word);

    word = _in->GetNextToken();
    light->m_colour[2] = atof(word);
    light->m_colour[3] = 0.0f;
  }
}

void LevelFile::ParseRoute(TextReader* _in, int _id)
{
  auto r = NEW Route(_id);

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("end", word) == 0)
      break;
    DEBUG_ASSERT(isdigit(word[0]));

    int type = atoi(word);
    LegacyVector3 pos;
    int buildingId = -1;

    switch (type)
    {
      case WayPoint::TypeGroundPos:
        pos.x = atof(_in->GetNextToken());
        pos.z = atof(_in->GetNextToken());
        break;
      case WayPoint::Type3DPos:
        pos.x = atof(_in->GetNextToken());
        pos.y = atof(_in->GetNextToken());
        pos.z = atof(_in->GetNextToken());
        break;
      case WayPoint::TypeBuilding:
        buildingId = atoi(_in->GetNextToken());
        break;
    }

    auto wp = NEW WayPoint(type, pos);
    if (buildingId != -1)
      wp->m_buildingId = buildingId;
    r->m_wayPoints.PutDataAtEnd(wp);
  }

  m_routes.PutDataAtEnd(r);
}

void LevelFile::ParseRoutes(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("Routes_EndDefinition", word) == 0)
      return;

    if (_stricmp("Route", word) == 0)
    {
      word = _in->GetNextToken();
      int id = atoi(word);
      DEBUG_ASSERT(id >= 0 && id < 10000);
      ParseRoute(_in, id);
    }
  }
}

void LevelFile::ParsePrimaryObjectives(TextReader* _in)
{
  while (_in->ReadLine())
  {
    char* word = _in->GetNextToken();

    if (_stricmp("PrimaryObjectives_EndDefinition", word) == 0)
      return;

    auto condition = NEW GlobalEventCondition;
    condition->m_type = condition->GetType(word);
    DEBUG_ASSERT(condition->m_type != -1);

    switch (condition->m_type)
    {
      case GlobalEventCondition::AlwaysTrue:
      case GlobalEventCondition::NotInLocation: DEBUG_ASSERT(false);
        break;

      case GlobalEventCondition::BuildingOffline:
      case GlobalEventCondition::BuildingOnline:
      {
        condition->m_locationId = g_app->m_globalWorld->GetLocationId(_in->GetNextToken());
        condition->m_id = atoi(_in->GetNextToken());
        DEBUG_ASSERT(condition->m_locationId != -1);
        break;
      }

      case GlobalEventCondition::ResearchOwned:
        condition->m_id = GlobalResearch::GetType(_in->GetNextToken());
        DEBUG_ASSERT(condition->m_id != -1);
        break;

      case GlobalEventCondition::DebugKey:
        condition->m_id = atoi(_in->GetNextToken());
        break;
    }

    if (_in->TokenAvailable())
    {
      char* stringId = _in->GetNextToken();
      condition->SetStringId(stringId);
    }

    if (_in->TokenAvailable())
    {
      char* cutScene = _in->GetNextToken();
      condition->SetCutScene(cutScene);
    }

    m_primaryObjectives.PutData(condition);
  }
}

void LevelFile::GenerateAutomaticObjectives()
{
  //
  // Create a NeverTrue objective for cutscene mission files

  if (m_primaryObjectives.Size() == 0)
  {
    auto objective = NEW GlobalEventCondition;
    objective->m_type = GlobalEventCondition::NeverTrue;
    m_primaryObjectives.PutData(objective);
  }

  //
  // Add secondary objectives for trunk ports and research items

  for (int i = 0; i < m_buildings.Size(); ++i)
  {
    Building* building = m_buildings.GetData(i);

    if (building->m_buildingType != BuildingType::TypeResearchItem && building->m_buildingType != BuildingType::TypeTrunkPort)
      continue;

    // Make sure this building isn't already in the primary objectives list
    bool found = false;
    for (int k = 0; k < m_primaryObjectives.Size(); ++k)
    {
      GlobalEventCondition* primaryObjective = m_primaryObjectives.GetData(k);
      if (primaryObjective->m_id == building->m_id.GetUniqueId())
      {
        found = true;
        break;
      }

      if (primaryObjective->m_type == GlobalEventCondition::ResearchOwned && building->m_buildingType == BuildingType::TypeResearchItem &&
        static_cast<ResearchItem*>(building)->m_researchType == primaryObjective->m_id)
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      int locationId = g_app->m_globalWorld->GetLocationIdFromMapFilename(m_mapFilename.c_str());

      if (building->m_buildingType == BuildingType::TypeResearchItem)
      {
        auto item = static_cast<ResearchItem*>(GetBuilding(building->m_id.GetUniqueId()));
        int currentLevel = g_app->m_globalWorld->m_research->CurrentLevel(item->m_researchType);
        if (currentLevel == 0 /*|| currentLevel < item->m_level*/)
        {
          // NOTE : We SHOULD really allow objectives to be created when the current research level
          // is below that of this ResearchItem - eg we have level 1 and this item is level 2.
          // However GlobalEventCondition isn't currently able to store the level data, so it
          // ends up being an auto-completed objective.
          auto condition = NEW GlobalEventCondition();
          condition->m_locationId = locationId;
          condition->m_type = GlobalEventCondition::ResearchOwned;
          condition->m_id = item->m_researchType;
          condition->SetStringId("objective_research");
          m_secondaryObjectives.PutData(condition);
        }
      }
      else if (building->m_buildingType == BuildingType::TypeTrunkPort)
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(building->m_id.GetUniqueId(), locationId);
        if (gb && !gb->m_online)
        {
          //
          // Is there a Control Tower that can enable this trunk port?
          bool towerFound = false;
          for (int c = 0; c < m_buildings.Size(); ++c)
          {
            Building* thisBuilding = m_buildings[c];
            if (thisBuilding->m_buildingType == BuildingType::TypeControlTower && thisBuilding->GetBuildingLink() == building->m_id.
              GetUniqueId())
            {
              towerFound = true;
              break;
            }
          }

          if (towerFound)
          {
            auto condition = NEW GlobalEventCondition;
            condition->m_locationId = locationId;
            condition->m_type = GlobalEventCondition::BuildingOnline;
            condition->m_id = building->m_id.GetUniqueId();
            condition->SetStringId("objective_capture_trunk");
            m_secondaryObjectives.PutData(condition);
          }
        }
      }
    }
  }
}

LevelFile::LevelFile()
  : m_landscapeColorFilename("landscape_default.bmp"),
  m_levelDifficulty(-1)
{
}

LevelFile::LevelFile(std::string_view _missionFilename, std::string_view _mapFilename)
  : m_missionFilename(_missionFilename),
    m_mapFilename(_mapFilename),
    m_landscapeColorFilename("landscape_default.bmp"),
  m_levelDifficulty(-1)
{
  // Make sure that the current game difficulty setting
  // is consistent with the preferences (it can become inconsistent
  // when a level is loaded that was saved with a different difficulty
  // level to what the preferences say).
  g_app->UpdateDifficultyFromPreferences();

  if (_missionFilename != "null")
    ParseMissionFile(m_missionFilename);
  ParseMapFile(m_mapFilename);

  GenerateAutomaticObjectives();
}

LevelFile::~LevelFile()
{
  m_cameraMounts.EmptyAndDelete();
  m_cameraAnimations.EmptyAndDelete();
  m_buildings.EmptyAndDelete();
  m_instantUnits.EmptyAndDelete();
  m_lights.EmptyAndDelete();
  m_routes.EmptyAndDelete();
  m_runningPrograms.EmptyAndDelete();
  m_primaryObjectives.EmptyAndDelete();
  m_secondaryObjectives.EmptyAndDelete();
}

Building* LevelFile::GetBuilding(int _id)
{
  for (int i = 0; i < m_buildings.Size(); ++i)
  {
    Building* building = m_buildings.GetData(i);
    if (building->m_id.GetUniqueId() == _id)
      return building;
  }
  return nullptr;
}

CameraMount* LevelFile::GetCameraMount(std::string_view _name)
{
  for (int i = 0; i < m_cameraMounts.Size(); ++i)
  {
    CameraMount* mount = m_cameraMounts.GetData(i);
    if (mount->m_name == _name)
      return mount;
  }
  return nullptr;
}

int LevelFile::GetCameraAnimId(std::string_view _name)
{
  for (int i = 0; i < m_cameraAnimations.Size(); ++i)
  {
    CameraAnimation* anim = m_cameraAnimations.GetData(i);
    if (anim->m_name == _name)
      return i;
  }
  return -1;
}

void LevelFile::RemoveBuilding(int _id)
{
  for (int i = 0; i < m_buildings.Size(); ++i)
  {
    if (m_buildings.ValidIndex(i))
    {
      Building* building = m_buildings.GetData(i);
      if (building->m_id.GetUniqueId() == _id)
      {
        m_buildings.RemoveData(i);
        delete building;
        break;
      }
    }
  }
}

Route* LevelFile::GetRoute(int _id)
{
  int size = m_routes.Size();
  for (int i = 0; i < size; ++i)
  {
    Route* route = m_routes.GetData(i);
    if (route->m_id == _id)
      return route;
  }

  return nullptr;
}

void LevelFile::ParseRunningPrograms(TextReader* _in)
{
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("RunningPrograms_EndDefinition", word) == 0)
      return;

    auto program = NEW RunningProgram();
    program->m_entityType = Entity::GetTypeId(word);
    program->m_count = atoi(_in->GetNextToken());
    program->m_state = atoi(_in->GetNextToken());
    program->m_data = atoi(_in->GetNextToken());
    program->m_waypointX = atof(_in->GetNextToken());
    program->m_waypointZ = atof(_in->GetNextToken());

    for (int i = 0; i < program->m_count; ++i)
    {
      program->m_positionX[i] = atof(_in->GetNextToken());
      program->m_positionZ[i] = atof(_in->GetNextToken());
      program->m_health[i] = atoi(_in->GetNextToken());
    }

    m_runningPrograms.PutData(program);
  }
}

void LevelFile::ParseDifficulty(TextReader* _in)
{
  m_levelDifficulty = -1;
  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;
    char* word = _in->GetNextToken();

    if (_stricmp("Difficulty_EndDefinition", word) == 0)
      return;
    if (_stricmp(word, "CreatedAsDifficulty") == 0)
    {
      // The difficulty setting is 1-based in the file, but 0-based internally
      m_levelDifficulty = atoi(_in->GetNextToken()) - 1;
      if (m_levelDifficulty < 0)
        m_levelDifficulty = 0;
    }
  }
}
