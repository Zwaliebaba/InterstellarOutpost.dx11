#include "pch.h"
#include "building.h"
#include "GameApp.h"
#include "NetworkClient.h"
#include "ai.h"
#include "anthill.h"
#include "blueprintstore.h"
#include "bridge.h"
#include "camera.h"
#include "cave.h"
#include "constructionyard.h"
#include "controltower.h"
#include "darwinian.h"
#include "entity_grid.h"
#include "explosion.h"
#include "factory.h"
#include "feedingtube.h"
#include "generator.h"
#include "generichub.h"
#include "globals.h"
#include "goddish.h"
#include "gunturret.h"
#include "incubator.h"
#include "laserfence.h"
#include "library.h"
#include "location.h"
#include "math_utils.h"
#include "matrix34.h"
#include "mine.h"
#include "obstruction_grid.h"
#include "ParticleSystem.h"
#include "powerstation.h"
#include "preferences.h"
#include "profiler.h"
#include "radardish.h"
#include "Renderer.h"
#include "researchitem.h"
#include "resource.h"
#include "rocket.h"
#include "safearea.h"
#include "scripttrigger.h"
#include "shape.h"
#include "soundsystem.h"
#include "spam.h"
#include "spawnpoint.h"
#include "spiritreceiver.h"
#include "staticshape.h"
#include "switch.h"
#include "team.h"
#include "text_stream_readers.h"
#include "triffid.h"
#include "trunkport.h"
#include "upgrade_port.h"
#include "wall.h"

Shape* Building::s_controlPad = nullptr;
ShapeMarker* Building::s_controlPadStatus = nullptr;

Building::Building(BuildingType _type)
  : WorldObject(ObjectType::Building),
  m_buildingType(_type),
  m_front(1, 0, 0),
  m_timeOfDeath(-1.0f),
  m_dynamic(false),
  m_isGlobal(false),
  m_destroyed(false),
  m_shape(nullptr)
{
  if (!s_controlPad)
  {
    s_controlPad = Resource::GetShape("controlpad.shp");
    DEBUG_ASSERT(s_controlPad);

    s_controlPadStatus = s_controlPad->m_rootFragment->LookupMarker("MarkerStatus");
    DEBUG_ASSERT(s_controlPadStatus);
  }

  m_id.SetTeamId(1);

  m_up = g_upVector;
}

void Building::Initialize(Building* _template)
{
  m_id = _template->m_id;
  m_pos = _template->m_pos;
  m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  m_front = _template->m_front;
  m_up = _template->m_up;
  m_dynamic = _template->m_dynamic;
  m_isGlobal = _template->m_isGlobal;
  m_destroyed = _template->m_destroyed;

  if (m_shape)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    m_centerPos = m_shape->CalculateCenter(mat);
    m_radius = m_shape->CalculateRadius(mat, m_centerPos);

    SetShapeLights(m_shape->m_rootFragment);
    SetShapePorts(m_shape->m_rootFragment);
  }
  else
  {
    m_centerPos = m_pos;
    m_radius = 13.0f;
  }

  GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_requestedLocationId);
  if (gb)
    m_id.SetTeamId(gb->m_teamId);

  g_app->m_soundSystem->TriggerBuildingEvent(this, "Create");
}

bool Building::Advance()
{
  if (m_destroyed)
    return true;

  EvaluatePorts();
  return false;
}

void Building::SetShape(Shape* _shape) { m_shape = _shape; }

void Building::SetShapeLights(ShapeFragment* _fragment)
{
  //
  // Enter all lights from this fragment

  int i;

  for (i = 0; i < _fragment->m_childMarkers.Size(); ++i)
  {
    ShapeMarker* marker = _fragment->m_childMarkers[i];
    if (strstr(marker->m_name.c_str(), "MarkerLight"))
      m_lights.PutData(marker);
  }

  //
  // Recurse to all child fragments

  for (i = 0; i < _fragment->m_childFragments.Size(); ++i)
  {
    ShapeFragment* fragment = _fragment->m_childFragments[i];
    SetShapeLights(fragment);
  }
}

void Building::SetShapePorts(ShapeFragment* _fragment)
{
  //
  // Enter all ports from this fragment

  int i;

  Matrix34 buildingMat(m_front, m_up, m_pos);

  for (i = 0; i < _fragment->m_childMarkers.Size(); ++i)
  {
    ShapeMarker* marker = _fragment->m_childMarkers[i];
    if (strstr(marker->m_name.c_str(), "MarkerPort"))
    {
      auto port = NEW BuildingPort();
      port->m_marker = marker;
      port->m_mat = marker->GetWorldMatrix(buildingMat);
      port->m_mat.pos = PushFromBuilding(port->m_mat.pos, 5.0f);
      port->m_mat.pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(port->m_mat.pos.x, port->m_mat.pos.z);

      for (int t = 0; t < NUM_TEAMS; ++t)
        port->m_counter[t] = 0;

      m_ports.PutData(port);
    }
  }

  //
  // Recurse to all child fragments

  for (i = 0; i < _fragment->m_childFragments.Size(); ++i)
  {
    ShapeFragment* fragment = _fragment->m_childFragments[i];
    SetShapePorts(fragment);
  }
}

void Building::Reprogram(float _complete) {}

void Building::ReprogramComplete()
{
  g_app->m_soundSystem->TriggerBuildingEvent(this, "ReprogramComplete");

  GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
  if (gb)
    gb->m_online = !gb->m_online;

  g_app->m_globalWorld->EvaluateEvents();
}

void Building::SetTeamId(int _teamId)
{
  m_id.SetTeamId(_teamId);

  GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
  if (gb)
    gb->m_teamId = _teamId;

  g_app->m_soundSystem->TriggerBuildingEvent(this, "ChangeTeam");
}

LegacyVector3 Building::PushFromBuilding(const LegacyVector3& pos, float _radius)
{
  START_PROFILE(g_app->m_profiler, "PushFromBuilding");

  LegacyVector3 result = pos;

  bool hit = false;
  if (DoesSphereHit(result, _radius))
    hit = true;

  if (hit)
  {
    LegacyVector3 pushForce = (m_pos - result).SetLength(2.0f);
    while (DoesSphereHit(result, _radius))
      result -= pushForce;
  }

  END_PROFILE(g_app->m_profiler, "PushFromBuilding");

  return result;
}

bool Building::IsInView() { return (g_app->m_camera->SphereInViewFrustum(m_centerPos, m_radius)); }

bool Building::PerformDepthSort(LegacyVector3& _centerPos) { return false; }

void Building::Render(float predictionTime)
{
  if (m_shape)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    m_shape->Render(predictionTime, mat);
  }
}

void Building::RenderAlphas(float predictionTime)
{
  RenderLights();
  RenderPorts();
}

void Building::RenderLights()
{
  if (m_id.GetTeamId() != 255 && m_lights.Size() > 0)
  {
    if ((g_app->m_networkClient->m_lastValidSequenceIdFromServer % 10) / 2 == m_id.GetTeamId())
    {
      for (int i = 0; i < m_lights.Size(); ++i)
      {
        ShapeMarker* marker = m_lights[i];
        Matrix34 rootMat(m_front, m_up, m_pos);
        Matrix34 worldMat = marker->GetWorldMatrix(rootMat);
        LegacyVector3 lightPos = worldMat.pos;

        float signalSize = 6.0f;
        LegacyVector3 camR = g_app->m_camera->GetRight();
        LegacyVector3 camU = g_app->m_camera->GetUp();

        if (m_id.GetTeamId() == 255)
          glColor3f(0.5f, 0.5f, 0.5f);
        else
          glColor3ubv(g_app->m_location->m_teams[m_id.GetTeamId()].m_colour.GetData());

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glDisable(GL_CULL_FACE);
        glDepthMask(false);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        for (int i = 0; i < 10; ++i)
        {
          float size = signalSize * static_cast<float>(i) / 10.0f;
          glBegin(GL_QUADS);
          glTexCoord2f(0.0f, 0.0f);
          glVertex3fv((lightPos - camR * size - camU * size).GetData());
          glTexCoord2f(1.0f, 0.0f);
          glVertex3fv((lightPos + camR * size - camU * size).GetData());
          glTexCoord2f(1.0f, 1.0f);
          glVertex3fv((lightPos + camR * size + camU * size).GetData());
          glTexCoord2f(0.0f, 1.0f);
          glVertex3fv((lightPos - camR * size + camU * size).GetData());
          glEnd();
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        glDepthMask(true);
        glEnable(GL_CULL_FACE);
        glDisable(GL_TEXTURE_2D);
      }
    }
  }
}

void Building::EvaluatePorts()
{
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    BuildingPort* port = m_ports[i];
    //port->m_mat = port->m_marker->GetWorldMatrix(buildingMat);
    port->m_occupant.SetInvalid();

    //
    // Look for a valid Darwinian near the port

    int numFound;
    if (g_app->m_location->m_entityGrid)
    {
      WorldObjectId* ids = g_app->m_location->m_entityGrid->GetNeighbours(port->m_mat.pos.x, port->m_mat.pos.z, 5.0f, &numFound);
      for (int i = 0; i < numFound; ++i)
      {
        WorldObjectId id = ids[i];
        Entity* entity = g_app->m_location->GetEntity(id);
        if (entity && entity->m_entityType == EntityType::TypeDarwinian)
        {
          auto darwinian = dynamic_cast<Darwinian*>(entity);
          if (darwinian->m_state == Darwinian::StateOperatingPort)
          {
            port->m_occupant = id;
            break;
          }
        }
      }
    }

    //
    // Update the operation counter

    for (int t = 0; t < NUM_TEAMS; ++t)
    {
      if (port->m_occupant.IsValid())
        port->m_counter[t] = 0;
      else
      {
        port->m_counter[t] -= 4;
        port->m_counter[t] = std::max(port->m_counter[t], 0);
      }
    }
  }
}

void Building::RenderPorts()
{
  START_PROFILE(g_app->m_profiler, "RenderPorts");

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail");

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    LegacyVector3 portPos;
    LegacyVector3 portFront;
    GetPortPosition(i, portPos, portFront);

    //
    // Render the port shape

    portPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(portPos.x, portPos.z) + 0.5f;
    LegacyVector3 portUp = g_upVector;
    Matrix34 mat(portFront, portUp, portPos);

    if (buildingDetail < 3)
    {
      g_app->m_renderer->SetObjectLighting();
      s_controlPad->Render(0.0f, mat);
      g_app->m_renderer->UnsetObjectLighting();
    }

    //
    // Render the status light

    float size = 6.0f;

    LegacyVector3 camR = g_app->m_camera->GetRight() * size;
    LegacyVector3 camU = g_app->m_camera->GetUp() * size;

    LegacyVector3 statusPos = s_controlPadStatus->GetWorldMatrix(mat).pos;

    if (GetPortOccupant(i).IsValid())
      glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
    else
      glColor4f(1.0f, 0.3f, 0.3f, 1.0f);

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((statusPos - camR - camU).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((statusPos + camR - camU).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((statusPos + camR + camU).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((statusPos - camR + camU).GetData());
    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
  }

  END_PROFILE(g_app->m_profiler, "RenderPorts");
}

void Building::Damage(float _damage) { g_app->m_soundSystem->TriggerBuildingEvent(this, "Damage"); }

void Building::Destroy(float _intensity)
{
  m_destroyed = true;

  Matrix34 mat(m_front, g_upVector, m_pos);
  for (int i = 0; i < 3; ++i)
    g_explosionManager.AddExplosion(m_shape, mat);
  g_app->m_location->Bang(m_pos, _intensity, _intensity / 4.0f);

  g_app->m_soundSystem->TriggerBuildingEvent(this, "Explode");

  for (int i = 0; i < static_cast<int>(_intensity / 4.0f); ++i)
  {
    LegacyVector3 vel(0.0f, 0.0f, 0.0f);
    vel.x += syncsfrand(100.0f);
    vel.y += syncsfrand(100.0f);
    vel.z += syncsfrand(100.0f);
    g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeExplosionCore, 100.0f);
  }
}

bool Building::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
  LegacyVector3* norm)
{
  if (m_shape)
  {
    RayPackage ray(_rayStart, _rayDir, _rayLen);
    Matrix34 transform(m_front, m_up, m_pos);
    return m_shape->RayHit(&ray, transform, true);
  }
  return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
}

bool Building::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  if (m_shape)
  {
    SpherePackage sphere(_pos, _radius);
    Matrix34 transform(m_front, m_up, m_pos);
    return m_shape->SphereHit(&sphere, transform);
  }
  float distance = (_pos - m_pos).Mag();
  return (distance <= _radius + m_radius);
}

bool Building::DoesShapeHit(Shape* _shape, Matrix34 _theTransform)
{
  if (m_shape)
  {
    Matrix34 ourTransform(m_front, m_up, m_pos);
    //return m_shape->ShapeHit( _shape, _theTransform, ourTransform, true );
    return _shape->ShapeHit(m_shape, ourTransform, _theTransform, true);
  }
  SpherePackage package(m_pos, m_radius);
  return _shape->SphereHit(&package, _theTransform, true);
}

int Building::GetBuildingLink() { return -1; }

void Building::SetBuildingLink(int _buildingId) {}

int Building::GetNumPorts() { return m_ports.Size(); }

int Building::GetNumPortsOccupied()
{
  int result = 0;
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    if (GetPortOccupant(i).IsValid())
      result++;
  }
  return result;
}

WorldObjectId Building::GetPortOccupant(int _portId)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    return port->m_occupant;
  }

  return WorldObjectId();
}

bool Building::GetPortPosition(int _portId, LegacyVector3& _pos, LegacyVector3& _front)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    _pos = port->m_mat.pos;
    _front = port->m_mat.f;
    return true;
  }

  return false;
}

void Building::OperatePort(int _portId, int _teamId)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    port->m_counter[_teamId]++;
    port->m_counter[_teamId] = std::min(port->m_counter[_teamId], 50);
  }
}

int Building::GetPortOperatorCount(int _portId, int _teamId)
{
  if (m_ports.ValidIndex(_portId))
  {
    BuildingPort* port = m_ports[_portId];
    return port->m_counter[_teamId];
  }

  return -1;
}

const char* Building::GetObjectiveCounter() { return ""; }

void Building::Read(TextReader* _in, bool _dynamic)
{
  char* word;

  int buildingId;
  int teamId;

  word = _in->GetNextToken();
  buildingId = atoi(word);
  word = _in->GetNextToken();
  m_pos.x = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  m_pos.z = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  teamId = atoi(word);
  word = _in->GetNextToken();
  m_front.x = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  m_front.z = static_cast<float>(atof(word));
  word = _in->GetNextToken();
  m_isGlobal = static_cast<bool>(atoi(word));

  m_front.Normalize();
  m_id.Set(teamId, UNIT_BUILDINGS, -1, buildingId);
  m_dynamic = _dynamic;
}

Building* Building::CreateBuilding(const char* _name)
{
  for (BuildingType i : RangeBuildingType())
  {
    if (_stricmp(_name, GetTypeName(i)) == 0)
      return CreateBuilding(i);
  }
  return nullptr;
}

Building* Building::CreateBuilding(BuildingType _type)
{
  Building* building = nullptr;

  switch (_type)
  {
  case BuildingType::TypeFactory:
    building = NEW Factory();
    break;
  case BuildingType::TypeCave:
    building = NEW Cave();
    break;
  case BuildingType::TypeRadarDish:
    building = NEW RadarDish();
    break;
  case BuildingType::TypeLaserFence:
    building = NEW LaserFence();
    break;
  case BuildingType::TypeControlTower:
    building = NEW ControlTower();
    break;
  case BuildingType::TypeGunTurret:
    building = NEW GunTurret();
    break;
  case BuildingType::TypeBridge:
    building = NEW Bridge();
    break;
  case BuildingType::TypePowerstation:
    building = NEW Powerstation();
    break;
  case BuildingType::TypeWall:
    building = NEW Wall();
    break;
  case BuildingType::TypeTrunkPort:
    building = NEW TrunkPort();
    break;
  case BuildingType::TypeResearchItem:
    building = NEW ResearchItem();
    break;
  case BuildingType::TypeLibrary:
    building = NEW Library();
    break;
  case BuildingType::TypeGenerator:
    building = NEW Generator();
    break;
  case BuildingType::TypePylon:
    building = NEW Pylon();
    break;
  case BuildingType::TypePylonStart:
    building = NEW PylonStart();
    break;
  case BuildingType::TypePylonEnd:
    building = NEW PylonEnd();
    break;
  case BuildingType::TypeSolarPanel:
    building = NEW SolarPanel();
    break;
  case BuildingType::TypeTrackLink:
    building = NEW TrackLink();
    break;
  case BuildingType::TypeTrackJunction:
    building = NEW TrackJunction();
    break;
  case BuildingType::TypeTrackStart:
    building = NEW TrackStart();
    break;
  case BuildingType::TypeTrackEnd:
    building = NEW TrackEnd();
    break;
  case BuildingType::TypeRefinery:
    building = NEW Refinery();
    break;
  case BuildingType::TypeMine:
    building = NEW Mine();
    break;
  case BuildingType::TypeYard:
    building = NEW ConstructionYard();
    break;
  case BuildingType::TypeDisplayScreen:
    building = NEW DisplayScreen();
    break;
  case BuildingType::TypeUpgradePort:
    building = NEW UpgradePort;
    break;
  case BuildingType::TypePrimaryUpgradePort:
    building = NEW PrimaryUpgradePort();
    break;
  case BuildingType::TypeIncubator:
    building = NEW Incubator();
    break;
  case BuildingType::TypeAntHill:
    building = NEW AntHill();
    break;
  case BuildingType::TypeSafeArea:
    building = NEW SafeArea();
    break;
  case BuildingType::TypeTriffid:
    building = NEW Triffid();
    break;
  case BuildingType::TypeSpiritReceiver:
    building = NEW SpiritReceiver();
    break;
  case BuildingType::TypeReceiverLink:
    building = NEW ReceiverLink();
    break;
  case BuildingType::TypeReceiverSpiritSpawner:
    building = NEW ReceiverSpiritSpawner();
    break;
  case BuildingType::TypeSpiritProcessor:
    building = NEW SpiritProcessor();
    break;
  case BuildingType::TypeSpawnPoint:
    building = NEW SpawnPoint();
    break;
  case BuildingType::TypeSpawnPopulationLock:
    building = NEW SpawnPopulationLock();
    break;
  case BuildingType::TypeSpawnPointMaster:
    building = NEW MasterSpawnPoint();
    break;
  case BuildingType::TypeSpawnLink:
    building = NEW SpawnLink();
    break;
  case BuildingType::TypeBlueprintStore:
    building = NEW BlueprintStore();
    break;
  case BuildingType::TypeBlueprintConsole:
    building = NEW BlueprintConsole();
    break;
  case BuildingType::TypeBlueprintRelay:
    building = NEW BlueprintRelay();
    break;
  case BuildingType::TypeAITarget:
    building = NEW AITarget();
    break;
  case BuildingType::TypeAISpawnPoint:
    building = NEW AISpawnPoint();
    break;
  case BuildingType::TypeScriptTrigger:
    building = NEW ScriptTrigger();
    break;
  case BuildingType::TypeSpam:
    building = NEW Spam();
    break;
  case BuildingType::TypeGodDish:
    building = NEW GodDish();
    break;
  case BuildingType::TypeStaticShape:
    building = NEW StaticShape();
    break;
  case BuildingType::TypeFuelGenerator:
    building = NEW FuelGenerator();
    break;
  case BuildingType::TypeFuelPipe:
    building = NEW FuelPipe();
    break;
  case BuildingType::TypeFuelStation:
    building = NEW FuelStation();
    break;
  case BuildingType::TypeEscapeRocket:
    building = NEW EscapeRocket();
    break;
  case BuildingType::TypeFenceSwitch:
    building = NEW FenceSwitch();
    break;
  case BuildingType::TypeDynamicHub:
    building = NEW DynamicHub();
    break;
  case BuildingType::TypeDynamicNode:
    building = NEW DynamicNode();
    break;
  case BuildingType::TypeFeedingTube:
    building = NEW FeedingTube();
    break;
  }

  if (_type == BuildingType::TypeRadarDish || _type == BuildingType::TypeControlTower || _type == BuildingType::TypeTrunkPort || _type == BuildingType::TypeIncubator || _type ==
    BuildingType::TypeDynamicHub || _type == BuildingType::TypeFenceSwitch)
    building->m_isGlobal = true;

  return building;
}

BuildingType Building::GetTypeId(const char* _name)
{
  for (BuildingType i : RangeBuildingType())
  {
    if (_stricmp(_name, GetTypeName(i)) == 0)
      return i;
  }
  return BuildingType::TypeInvalid;
}

const char* Building::GetTypeName(BuildingType _type)
{
  static const char* buildingNames[] = {
    "Factory",                              // These must be in the
    "Cave",                                 // Same order as defined
    "RadarDish",                            // in building.h
    "LaserFence",
    "ControlTower",
    "GunTurret",
    "Bridge",
    "Powerstation",
    "Wall",
    "TrunkPort",
    "ResearchItem",
    "Library",
    "Generator",
    "Pylon",
    "PylonStart",
    "PylonEnd",
    "SolarPanel",
    "TrackLink",
    "TrackJunction",
    "TrackStart",
    "TrackEnd",
    "Refinery",
    "Mine",
    "Yard",
    "DisplayScreen",
    "UpgradePort",
    "PrimaryUpgrade",
    "Incubator",
    "AntHill",
    "SafeArea",
    "Triffid",
    "SpiritReceiver",
    "ReceiverLink",
    "SpiritSpawner",
    "SpiritProcessor",
    "SpawnPoint",
    "SpawnPopulationLock",
    "SpawnPointMaster",
    "SpawnLink",
    "AITarget",
    "AISpawnPoint",
    "BlueprintStore",
    "BlueprintConsole",
    "BlueprintRelay",
    "ScriptTrigger",
    "Spam",
    "GodDish",
    "StaticShape",
    "FuelGenerator",
    "FuelPipe",
    "FuelStation",
    "EscapeRocket",
    "FenceSwitch",
    "DynamicHub",
    "DynamicNode",
    "FeedingTube"
  };
  static_assert(SizeOfBuildingType() == _countof(buildingNames), "Incorrect Array");
  
  DEBUG_ASSERT(I(_type) < _countof(buildingNames));
  return buildingNames[I(_type)];
}

std::string Building::GetTypeNameTranslated(BuildingType _type)
{
  const char* typeName = GetTypeName(_type);

  char stringId[256];
  sprintf(stringId, "buildingname_%s", typeName);

  return Strings::Get(stringId);
}
