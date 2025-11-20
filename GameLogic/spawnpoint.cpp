#include "pch.h"
#include "spawnpoint.h"
#include "GameApp.h"
#include "camera.h"
#include "entity_grid.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "shape.h"
#include "team.h"
#include "text_stream_readers.h"

SpawnBuilding::SpawnBuilding(BuildingType _type)
  : Building(_type),
    m_spiritLink(nullptr),
    m_visibilityRadius(0.0f) {}

SpawnBuilding::~SpawnBuilding()
{
  m_links.EmptyAndDelete();
}

void SpawnBuilding::Initialize(Building* _template)
{
  Building::Initialize(_template);

  auto spawn = static_cast<SpawnBuilding*>(_template);
  for (int i = 0; i < spawn->m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = spawn->m_links[i];
    SetBuildingLink(link->m_targetBuildingId);
  }
}

bool SpawnBuilding::IsInView()
{
  if (m_visibilityRadius == 0.0f)
  {
    if (m_links.Size() == 0)
    {
      m_visibilityRadius = m_radius;
      m_visibilityMidpoint = m_centerPos;
      return Building::IsInView();
    }
    m_visibilityMidpoint = m_centerPos;
    int numLinks = 1;
    m_visibilityRadius = 0.0f;

    // Find mid point

    for (int i = 0; i < m_links.Size(); ++i)
    {
      SpawnBuildingLink* link = m_links[i];
      auto building = static_cast<SpawnBuilding*>(g_app->m_location->GetBuilding(link->m_targetBuildingId));
      if (building)
      {
        m_visibilityMidpoint += building->m_centerPos;
        ++numLinks;
      }
    }

    m_visibilityMidpoint /= static_cast<float>(numLinks);

    // Find radius

    for (int i = 0; i < m_links.Size(); ++i)
    {
      SpawnBuildingLink* link = m_links[i];
      auto building = static_cast<SpawnBuilding*>(g_app->m_location->GetBuilding(link->m_targetBuildingId));
      if (building)
      {
        float distance = (building->m_centerPos - m_visibilityMidpoint).Mag();
        distance += building->m_radius / 2.0f;
        m_visibilityRadius = std::max(m_visibilityRadius, distance);
      }
    }
  }

  // Determine visibility

  return g_app->m_camera->SphereInViewFrustum(m_visibilityMidpoint, m_visibilityRadius);
}

void SpawnBuilding::RenderSpirit(const LegacyVector3& _pos)
{
  LegacyVector3 pos = _pos;

  int innerAlpha = 255;
  int outerAlpha = 150;
  int spiritInnerSize = 2;
  int spiritOuterSize = 6;

  float size = spiritInnerSize;
  glColor4ub(150, 50, 25, innerAlpha);

  glBegin(GL_QUADS);
  glVertex3fv((pos - g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetRight() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos - g_app->m_camera->GetRight() * size).GetData());
  glEnd();

  size = spiritOuterSize;
  glColor4ub(150, 50, 25, outerAlpha);

  glBegin(GL_QUADS);
  glVertex3fv((pos - g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetRight() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos - g_app->m_camera->GetRight() * size).GetData());
  glEnd();
}

void SpawnBuilding::RenderAlphas(const float _predictionTime)
{
  LegacyVector3 ourPos = GetSpiritLink();

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    auto building = static_cast<SpawnBuilding*>(g_app->m_location->GetBuilding(link->m_targetBuildingId));
    if (building)
    {
      LegacyVector3 theirPos = building->GetSpiritLink();

      LegacyVector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
      LegacyVector3 ourPosRight = camToOurPos ^ (theirPos - ourPos);

      LegacyVector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
      LegacyVector3 theirPosRight = camToTheirPos ^ (theirPos - ourPos);

      glDisable(GL_CULL_FACE);
      glDepthMask(false);
      glColor4f(0.9f, 0.9f, 0.5f, 1.0f);

      float size = 0.5f;

      if (buildingDetail == 1)
      {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laser.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        size = 1.0f;
      }

      theirPosRight.SetLength(size);
      ourPosRight.SetLength(size);

      glBegin(GL_QUADS);
      glTexCoord2f(0.1f, 0);
      glVertex3fv((ourPos - ourPosRight).GetData());
      glTexCoord2f(0.1f, 1);
      glVertex3fv((ourPos + ourPosRight).GetData());
      glTexCoord2f(0.9f, 1);
      glVertex3fv((theirPos + theirPosRight).GetData());
      glTexCoord2f(0.9f, 0);
      glVertex3fv((theirPos - theirPosRight).GetData());
      glEnd();

      glDisable(GL_TEXTURE_2D);

      //
      // Render spirits in transit

      for (int j = 0; j < link->m_spirits.Size(); ++j)
      {
        SpawnBuildingSpirit* spirit = link->m_spirits[j];
        float predictedProgress = spirit->m_currentProgress + _predictionTime;
        if (predictedProgress >= 0.0f && predictedProgress <= 1.0f)
        {
          LegacyVector3 position = ourPos + (theirPos - ourPos) * predictedProgress;
          RenderSpirit(position);
        }
      }
    }
  }

  Building::RenderAlphas(_predictionTime);
}

void SpawnBuilding::SetBuildingLink(const int _buildingId)
{
  auto link = new SpawnBuildingLink();
  link->m_targetBuildingId = _buildingId;
  m_links.PutData(link);
}

void SpawnBuilding::ClearLinks() { m_links.EmptyAndDelete(); }

LList<int>* SpawnBuilding::ExploreLinks()
{
  auto result = new LList<int>();

  if (m_buildingType == BuildingType::TypeSpawnPoint)
    result->PutData(m_id.GetUniqueId());

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    link->m_targets.Empty();

    auto target = static_cast<SpawnBuilding*>(g_app->m_location->GetBuilding(link->m_targetBuildingId));
    if (target)
    {
      LList<int>* availableLinks = target->ExploreLinks();
      for (int j = 0; j < availableLinks->Size(); ++j)
      {
        int thisLink = availableLinks->GetData(j);
        result->PutData(thisLink);
        link->m_targets.PutData(thisLink);
      }
      delete availableLinks;
    }
  }

  return result;
}

LegacyVector3 SpawnBuilding::GetSpiritLink()
{
  if (!m_spiritLink)
  {
    m_spiritLink = m_shape->m_rootFragment->LookupMarker("MarkerSpiritLink");
    DEBUG_ASSERT(m_spiritLink);
  }

  Matrix34 mat(m_front, g_upVector, m_pos);

  return m_spiritLink->GetWorldMatrix(mat).pos;
}

void SpawnBuilding::TriggerSpirit(SpawnBuildingSpirit* _spirit)
{
  int numAdds = 0;

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    for (int j = 0; j < link->m_targets.Size(); ++j)
    {
      int thisLink = link->m_targets[j];
      if (link->m_targets[j] == _spirit->m_targetBuildingId)
      {
        link->m_spirits.PutData(_spirit);
        _spirit->m_currentProgress = 0.0f;
        ++numAdds;
      }
    }
  }

  if (numAdds == 0)
  {
    // We have nowhere else to go
    // Or maybe we have arrived
    delete _spirit;
  }
}

bool SpawnBuilding::Advance()
{
  //
  // Advance all spirits in transit

  for (int i = 0; i < m_links.Size(); ++i)
  {
    SpawnBuildingLink* link = m_links[i];
    for (int j = 0; j < link->m_spirits.Size(); ++j)
    {
      SpawnBuildingSpirit* spirit = link->m_spirits[j];
      spirit->m_currentProgress += SERVER_ADVANCE_PERIOD;
      if (spirit->m_currentProgress >= 1.0f)
      {
        link->m_spirits.RemoveData(j);
        --j;
        auto building = static_cast<SpawnBuilding*>(g_app->m_location->GetBuilding(link->m_targetBuildingId));
        if (building)
          building->TriggerSpirit(spirit);
      }
    }
  }

  return Building::Advance();
}

void SpawnBuilding::Render(const float _predictionTime) { Building::Render(_predictionTime); }

void SpawnBuilding::Read(TextReader* _in, const bool _dynamic)
{
  Building::Read(_in, _dynamic);

  while (_in->TokenAvailable())
  {
    int nextLink = atoi(_in->GetNextToken());
    SetBuildingLink(nextLink);
  }
}

SpawnLink::SpawnLink()
  : SpawnBuilding(BuildingType::TypeSpawnLink)
{
  Building::SetShape(Resource::GetShape("spawnlink.shp"));
}

int MasterSpawnPoint::s_masterSpawnPointId = -1;

MasterSpawnPoint::MasterSpawnPoint()
  : SpawnBuilding(BuildingType::TypeSpawnPointMaster),
    m_exploreLinks(false)
{
  Building::SetShape(Resource::GetShape("masterspawnpoint.shp"));
}

MasterSpawnPoint* MasterSpawnPoint::GetMasterSpawnPoint()
{
  Building* building = g_app->m_location->GetBuilding(s_masterSpawnPointId);
  if (building && building->m_buildingType == BuildingType::TypeSpawnPointMaster)
    return dynamic_cast<MasterSpawnPoint*>(building);

  return nullptr;
}

void MasterSpawnPoint::RequestSpirit(const int _targetBuildingId)
{
  auto spirit = new SpawnBuildingSpirit();
  spirit->m_targetBuildingId = _targetBuildingId;
  TriggerSpirit(spirit);
}

bool MasterSpawnPoint::Advance()
{
  s_masterSpawnPointId = m_id.GetUniqueId();

  //
  // Explore our links first time through

  if (!m_exploreLinks)
  {
    m_exploreLinks = true;
    float startTime = GetHighResTime();
    ExploreLinks();
    float timeTaken = GetHighResTime() - startTime;
    DebugTrace("Time to Explore all Spawn Point links : %d ms\n", static_cast<int>(timeTaken * 1000.0f));
  }

  //
  // Accelerate spirits to heaven

  if (m_isGlobal)
  {
    for (int i = 0; i < g_app->m_location->m_spirits.Size(); ++i)
    {
      if (g_app->m_location->m_spirits.ValidIndex(i))
      {
        Spirit* s = g_app->m_location->m_spirits.GetPointer(i);
        if (s->m_state == Spirit::StateBirth || s->m_state == Spirit::StateFloating)
          s->SkipStage();
      }
    }
  }

  //
  // Have the red guys been wiped out?

  if (g_app->m_location->m_teams[1].m_teamType != Team::TeamTypeUnused)
  {
    int numRed = g_app->m_location->m_teams[1].m_others.NumUsed();
    if (numRed < 10)
    {
      GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
      if (gb)
        gb->m_online = true;
    }
  }

  return SpawnBuilding::Advance();
}

void MasterSpawnPoint::Render(const float _predictionTime)
{
  if (m_isGlobal)
    SpawnBuilding::Render(_predictionTime);
}

void MasterSpawnPoint::RenderAlphas(const float _predictionTime)
{
  if (m_isGlobal)
    SpawnBuilding::RenderAlphas(_predictionTime);
}

const char* MasterSpawnPoint::GetObjectiveCounter()
{
  static char result[256];

  if (g_app->m_location->m_teams[1].m_teamType != Team::TeamTypeUnused)
  {
    int numRed = g_app->m_location->m_teams[1].m_others.NumUsed();
    sprintf(result, "%s : %d", Strings::Get("objective_redpopulation").c_str(), numRed);
  }
  else
    sprintf(result, "%s", Strings::Get("objective_redpopulation").c_str());

  return result;
}

// ============================================================================

SpawnPoint::SpawnPoint()
  : SpawnBuilding(BuildingType::TypeSpawnPoint),
    m_evaluateTimer(0.0f),
    m_spawnTimer(0.0f),
    m_populationLock(-1),
    m_numFriendsNearby(0)
{
  Building::SetShape(Resource::GetShape("spawnpoint.shp"));
  m_doorMarker = m_shape->m_rootFragment->LookupMarker("MarkerDoor");

  m_evaluateTimer = syncfrand(2.0f);
  m_spawnTimer = 2.0f + syncfrand(2.0f);
}

bool SpawnPoint::PopulationLocked()
{
  //
  // If we haven't yet looked up a nearby Pop lock,
  // do so now

  if (m_populationLock == -1)
  {
    m_populationLock = -2;

    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building && building->m_buildingType == BuildingType::TypeSpawnPopulationLock)
        {
          auto lock = dynamic_cast<SpawnPopulationLock*>(building);
          float distance = (building->m_pos - m_pos).Mag();
          if (distance < lock->m_searchRadius)
          {
            m_populationLock = lock->m_id.GetUniqueId();
            break;
          }
        }
      }
    }
  }

  //
  // If we are inside a Population Lock, query it now

  if (m_populationLock > 0)
  {
    auto lock = dynamic_cast<SpawnPopulationLock*>(g_app->m_location->GetBuilding(m_populationLock));
    if (lock && m_id.GetTeamId() != 255 && lock->m_teamCount[m_id.GetTeamId()] >= lock->m_maxPopulation)
      return true;
  }

  return false;
}

void SpawnPoint::RecalculateOwnership()
{
  int teamCount[NUM_TEAMS];
  memset(teamCount, 0, NUM_TEAMS * sizeof(int));

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    WorldObjectId id = GetPortOccupant(i);
    if (id.IsValid())
      teamCount[id.GetTeamId()]++;
  }

  int winningTeam = -1;
  for (int i = 0; i < NUM_TEAMS; ++i)
  {
    if (teamCount[i] > 2 && winningTeam == -1)
      winningTeam = i;
    else if (winningTeam != -1 && teamCount[i] > 2 && teamCount[i] > teamCount[winningTeam])
      winningTeam = i;
  }

  if (winningTeam == -1)
    SetTeamId(255);
  else
    SetTeamId(winningTeam);
}

void SpawnPoint::TriggerSpirit(SpawnBuildingSpirit* _spirit)
{
  if (m_id.GetTeamId() == 0)
    int b = 10;

  if (m_id.GetUniqueId() == _spirit->m_targetBuildingId)
  {
    // This spirit has arrived at its destination
    if (m_id.GetTeamId() != 255)
    {
      Matrix34 mat(m_front, m_up, m_pos);
      Matrix34 doorMat = m_doorMarker->GetWorldMatrix(mat);
      g_app->m_location->SpawnEntities(doorMat.pos, m_id.GetTeamId(), -1, EntityType::TypeDarwinian, 1, g_zeroVector, 0.0f);
    }

    delete _spirit;
  }
  else
    SpawnBuilding::TriggerSpirit(_spirit);
}

bool SpawnPoint::PerformDepthSort(LegacyVector3& _centerPos)
{
  _centerPos = m_centerPos;
  return true;
}

bool SpawnPoint::Advance()
{
  //
  // Re-evalulate our situation

  m_evaluateTimer -= SERVER_ADVANCE_PERIOD;
  if (m_evaluateTimer <= 0.0f)
  {
    START_PROFILE(g_app->m_profiler, "Evaluate");

    RecalculateOwnership();
    m_evaluateTimer = 2.0f;

    END_PROFILE(g_app->m_profiler, "Evaluate");
  }

  //
  // Time to request more spirits for our Darwinians?

  START_PROFILE(g_app->m_profiler, "SpawnDarwinians");
  if (m_id.GetTeamId() != 255 && !PopulationLocked())
  {
    m_spawnTimer -= SERVER_ADVANCE_PERIOD;
    if (m_spawnTimer <= 0.0f)
    {
      MasterSpawnPoint* masterSpawn = MasterSpawnPoint::GetMasterSpawnPoint();
      if (masterSpawn)
        masterSpawn->RequestSpirit(m_id.GetUniqueId());

      float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

      m_spawnTimer = 2.0f + 5.0f * (1.0f - fractionOccupied);

      // Reds spawn a bit quicker
      if (m_id.GetTeamId() == 1)
        m_spawnTimer -= 1.0 * (g_app->m_difficultyLevel / 10.0);
      else
        m_spawnTimer -= 0.5 * (g_app->m_difficultyLevel / 10.0);
    }
  }
  END_PROFILE(g_app->m_profiler, "SpawnDarwinians");

  return SpawnBuilding::Advance();
}

void SpawnPoint::Render(const float _predictionTime) { SpawnBuilding::Render(_predictionTime); }

void SpawnPoint::RenderAlphas(const float _predictionTime)
{
  SpawnBuilding::RenderAlphas(_predictionTime);

  LegacyVector3 camUp = g_app->m_camera->GetUp();
  LegacyVector3 camRight = g_app->m_camera->GetRight();

  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\cloudyglow.bmp"));

  float timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0f;

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
  int maxBlobs = 20;
  if (buildingDetail == 2)
    maxBlobs = 10;
  if (buildingDetail == 3)
    maxBlobs = 0;

  float alpha = GetNumPortsOccupied() / static_cast<float>(GetNumPorts());

  for (int i = 0; i < maxBlobs; ++i)
  {
    LegacyVector3 pos = m_centerPos;
    pos += LegacyVector3(0, 25, 0);
    pos.x += sinf(timeIndex + i) * i * 0.7f;
    pos.y += cosf(timeIndex + i) * sinf(i * 10) * 12;
    pos.z += cosf(timeIndex + i) * i * 0.7f;

    float size = 10.0f + sinf(timeIndex + i * 10) * 10.0f;
    size = std::max(size, 5.0f);

    glColor4f(0.6f, 0.2f, 0.1f, alpha);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((pos - camRight * size + camUp * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((pos + camRight * size + camUp * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((pos + camRight * size - camUp * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((pos - camRight * size - camUp * size).GetData());
    glEnd();
  }

  glDisable(GL_TEXTURE_2D);
  glDepthMask(true);
}

void SpawnPoint::RenderPorts()
{
  glDisable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBegin(GL_QUADS);

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    LegacyVector3 portPos;
    LegacyVector3 portFront;
    GetPortPosition(i, portPos, portFront);

    LegacyVector3 portUp = g_upVector;
    Matrix34 mat(portFront, portUp, portPos);

    //
    // Render the status light

    float size = 6.0f;
    LegacyVector3 camR = g_app->m_camera->GetRight() * size;
    LegacyVector3 camU = g_app->m_camera->GetUp() * size;

    LegacyVector3 statusPos = s_controlPadStatus->GetWorldMatrix(mat).pos;
    statusPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
    statusPos.y += 5.0f;

    WorldObjectId occupantId = GetPortOccupant(i);
    if (!occupantId.IsValid())
      glColor4ub(150, 150, 150, 255);
    else
    {
      RGBAColor teamColor = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
      glColor4ubv(teamColor.GetData());
    }

    glTexCoord2i(0, 0);
    glVertex3fv((statusPos - camR - camU).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((statusPos + camR - camU).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((statusPos + camR + camU).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((statusPos - camR + camU).GetData());
  }

  glEnd();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glDepthMask(true);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}

// ============================================================================

float SpawnPopulationLock::s_overpopulationTimer = 0.0f;
int SpawnPopulationLock::s_overpopulation = 0;

SpawnPopulationLock::SpawnPopulationLock()
  : Building(BuildingType::TypeSpawnPopulationLock),
    m_searchRadius(500.0f),
    m_maxPopulation(100),
    m_originalMaxPopulation(100),
    m_recountTeamId(-1)
{
  memset(m_teamCount, 0, NUM_TEAMS * sizeof(int));

  m_recountTimer = syncfrand(10.0f);
  m_recountTeamId = 10;
}

void SpawnPopulationLock::Initialize(Building* _template)
{
  Building::Initialize(_template);

  auto lock = static_cast<SpawnPopulationLock*>(_template);
  m_searchRadius = lock->m_searchRadius;
  m_maxPopulation = lock->m_maxPopulation;

  m_originalMaxPopulation = m_maxPopulation;
}

bool SpawnPopulationLock::Advance()
{
  //
  // Every once in a while, have a look at the global over-population rate
  // And reduce all local maxPopulations accordingly
  // This only really applies to Green darwinians, as the player can arrange
  // it so an unlimited army gathers on a single island

  if (GetHighResTime() > s_overpopulationTimer)
  {
    s_overpopulationTimer = GetHighResTime() + 1.0f;

    int totalOverpopulation = 0;

    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building && building->m_buildingType == BuildingType::TypeSpawnPopulationLock)
        {
          auto lock = dynamic_cast<SpawnPopulationLock*>(building);
          if (lock->m_teamCount[0] > lock->m_originalMaxPopulation)
            totalOverpopulation += (lock->m_teamCount[0] - lock->m_originalMaxPopulation);
        }
      }
    }

    s_overpopulation = totalOverpopulation / 2;
  }

  m_maxPopulation = m_originalMaxPopulation - s_overpopulation;
  m_maxPopulation = std::max(m_maxPopulation, 0);

  //
  // Recount the number of entities nearby

  m_recountTimer -= SERVER_ADVANCE_PERIOD;
  if (m_recountTimer <= 0.0f)
  {
    m_recountTimer = 10.0f;
    m_recountTeamId = 10;
  }

  int potentialTeamId = static_cast<int>(m_recountTimer);
  if (potentialTeamId < m_recountTeamId && potentialTeamId < NUM_TEAMS)
  {
    m_recountTeamId = potentialTeamId;
    m_teamCount[potentialTeamId] = g_app->m_location->m_entityGrid->
                                          GetNumFriends(m_pos.x, m_pos.z, m_searchRadius, m_recountTeamId);
  }

  return Building::Advance();
}

void SpawnPopulationLock::Read(TextReader* _in, const bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_searchRadius = atof(_in->GetNextToken());
  m_maxPopulation = atoi(_in->GetNextToken());
}

bool SpawnPopulationLock::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool SpawnPopulationLock::DoesShapeHit(Shape* _shape, Matrix34 _transform) { return false; }
