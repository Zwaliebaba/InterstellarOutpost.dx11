#include "pch.h"
#include "controltower.h"
#include "GameApp.h"
#include "camera.h"
#include "NetworkClient.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "matrix34.h"
#include "ParticleSystem.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"
#include "team.h"
#include "text_stream_readers.h"
#include "trunkport.h"

Shape* ControlTower::s_dishShape = nullptr;

ControlTower::ControlTower()
  : Building(BuildingType::TypeControlTower),
    m_ownership(100.0f),
    m_controlBuildingId(-1),
    m_checkTargetTimer(0.0f)
{
  m_radius = 4.0f;

  Building::SetShape(Resource::GetShape("controltower.shp"));
  m_lightPos = m_shape->m_rootFragment->LookupMarker("MarkerLightPos");
  m_dishPos = m_shape->m_rootFragment->LookupMarker("MarkerDishPos");

  for (int i = 0; i < 3; ++i)
  {
    m_beingReprogrammed[i] = false;
    char markerName[64];

    sprintf(markerName, "MarkerReprogrammer%d", i);
    m_reprogrammer[i] = m_shape->m_rootFragment->LookupMarker(markerName);

    sprintf(markerName, "MarkerConsole%d", i);
    m_console[i] = m_shape->m_rootFragment->LookupMarker(markerName);
  }

  if (!s_dishShape)
    s_dishShape = Resource::GetShape("controltowerdish.shp");
}

void ControlTower::Initialize(Building* _template)
{
  Building::Initialize(_template);

  m_controlBuildingId = static_cast<ControlTower*>(_template)->m_controlBuildingId;
}

bool ControlTower::Advance()
{
  //
  // Calculate our Dish matrix (once only, during first Advance)
  // Also look to see if our building is already captured

  if (m_dishMatrix == Matrix34())
  {
    Building* targetBuilding = g_app->m_location->GetBuilding(m_controlBuildingId);
    if (targetBuilding)
    {
      Matrix34 mat(m_front, g_upVector, m_pos);
      LegacyVector3 dishPos = m_dishPos->GetWorldMatrix(mat).pos;
      LegacyVector3 dishFront = (dishPos - targetBuilding->m_centerPos).Normalize();
      LegacyVector3 dishRight = dishFront ^ g_upVector;
      LegacyVector3 dishUp = dishRight ^ dishFront;
      m_dishMatrix = Matrix34(dishFront, dishUp, dishPos);

      if (targetBuilding->m_id.GetTeamId() == 255)
        m_ownership = 0.0f;
      else
        m_ownership = 100.0f;
    }
  }

  //
  // If we are connected to a TrunkPort, somebody else may have opened that port from another map.
  // Every once in a while, check and turn on if it has happened

  m_checkTargetTimer -= SERVER_ADVANCE_PERIOD;

  if (m_checkTargetTimer <= 0.0f)
  {
    Building* building = g_app->m_location->GetBuilding(m_controlBuildingId);
    if (building && building->m_buildingType == BuildingType::TypeTrunkPort && m_id.GetTeamId() != 2)
    {
      auto tp = static_cast<TrunkPort*>(building);
      if (tp->m_openTimer > 0.0f)
      {
        m_id.SetTeamId(2);
        m_ownership = 100.0f;
      }
    }
    m_checkTargetTimer = 2.0f;
  }

  //
  // Spawn particles if we are being reprogrammed

  for (int i = 0; i < 3; ++i)
  {
    if (m_beingReprogrammed[i])
    {
      Matrix34 rootMat(m_front, g_upVector, m_pos);
      Matrix34 worldMat = m_console[i]->GetWorldMatrix(rootMat);
      LegacyVector3 particleVel = worldMat.pos - m_pos;
      particleVel += LegacyVector3(sfrand() * 10.0f, sfrand() * 5.0f, sfrand() * 10.0f);
      g_app->m_particleSystem->CreateParticle(worldMat.pos, particleVel, Particle::TypeBlueSpark);
    }
  }

  return Building::Advance();
}

int ControlTower::GetAvailablePosition(LegacyVector3& _pos, LegacyVector3& _front)
{
  for (int i = 0; i < 3; ++i)
  {
    if (!m_beingReprogrammed[i])
    {
      Matrix34 rootMat(m_front, g_upVector, m_pos);
      Matrix34 worldMat = m_reprogrammer[i]->GetWorldMatrix(rootMat);

      _pos = worldMat.pos;
      _front = worldMat.f;

      return i;
    }
  }

  return -1;
}

void ControlTower::GetConsolePosition(int _position, LegacyVector3& _pos)
{
  DEBUG_ASSERT(_position >= 0 && _position < 3);

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_console[_position]->GetWorldMatrix(rootMat);

  _pos = worldMat.pos;
}

void ControlTower::BeginReprogram(int _position)
{
  DEBUG_ASSERT(!m_beingReprogrammed[_position]);
  m_beingReprogrammed[_position] = true;
}

bool ControlTower::Reprogram(int _teamId)
{
  if (_teamId != m_id.GetTeamId())
  {
    // Removing someone elses control
    m_ownership -= 0.1f;
    if (m_ownership <= 0.0f)
    {
      m_id.SetTeamId(_teamId);
      Building* targetBuilding = g_app->m_location->GetBuilding(m_controlBuildingId);
      if (targetBuilding && targetBuilding->m_id.GetTeamId() != m_id.GetTeamId())
        targetBuilding->SetTeamId(m_id.GetTeamId());
    }
  }
  else
  {
    // Increasing our own control
    if (m_ownership < 100.0f)
    {
      m_ownership += 0.1f;
      if (m_ownership > 100.0f)
        m_ownership = 100.0f;

      Building* targetBuilding = g_app->m_location->GetBuilding(m_controlBuildingId);
      if (targetBuilding)
      {
        targetBuilding->Reprogram(m_ownership);

        if (m_ownership == 100.0f)
        {
          g_app->m_soundSystem->TriggerBuildingEvent(this, "ReprogramComplete");
          //g_app->m_sepulveda->Say("building_captured");
          targetBuilding->ReprogramComplete();
          SetTeamId(_teamId);
          g_app->m_globalWorld->m_research->GiveResearchPoints(GLOBALRESEARCH_POINTS_CONTROLTOWER);

          GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
          if (gb)
          {
            gb->m_online = true;
            g_app->m_globalWorld->EvaluateEvents();
          }
          return true;
        }
      }
    }
    else
      return true;
  }

  return false;
}

void ControlTower::EndReprogram(int _position)
{
  DEBUG_ASSERT(m_beingReprogrammed[_position]);
  m_beingReprogrammed[_position] = false;
}

void ControlTower::Render(float _predictionTime)
{
  Building::Render(_predictionTime);

  //
  // Render our dish

  s_dishShape->Render(_predictionTime, m_dishMatrix);
}

bool ControlTower::IsInView()
{
  if (Building::IsInView())
    return true;

  //
  // Check against the tall thin control line to heaven

  LegacyVector3 towerPos = m_pos;
  towerPos.y = g_app->m_camera->GetPos().y;
  return g_app->m_camera->PosInViewFrustum(towerPos);
}

void ControlTower::RenderAlphas(float _predictionTime)
{
  Building::RenderAlphas(_predictionTime);

  //
  // Control lines will be bright when we are near a control tower
  // And dim when we are not
  // Recalculate our distance to the nearest control tower once per second

  static int s_lastRecalculation = 0.0f;
  static float s_distanceScale = 0.0f;
  static float s_desiredDistanceScale = 0.0f;

  if (static_cast<int>(GetHighResTime()) > s_lastRecalculation)
  {
    s_lastRecalculation = static_cast<int>(GetHighResTime());

    float nearest = 99999.9f;
    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building && building->m_buildingType == BuildingType::TypeControlTower)
        {
          float camDist = (building->m_pos - g_app->m_camera->GetPos()).Mag();
          if (camDist < nearest)
            nearest = camDist;
        }
      }
    }

    if (nearest < 200.0f)
      s_desiredDistanceScale = 1.0f;
    else
      s_desiredDistanceScale = 0.1f;
  }

  if (s_desiredDistanceScale > s_distanceScale)
  {
    s_distanceScale = (s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.1f) + (s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD *
      0.1f));
  }
  else
  {
    s_distanceScale = (s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.03f) + (s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD *
      0.03f));
  }

  //
  // Pre-compute some positions and shit

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_lightPos->GetWorldMatrix(rootMat);
  LegacyVector3 lightPos = worldMat.pos;

  LegacyVector3 camR = g_app->m_camera->GetRight();
  LegacyVector3 camU = g_app->m_camera->GetUp();

  RGBAColor colour;
  if (m_id.GetTeamId() == 255)
    colour.Set(128, 128, 128, 255);
  else
    colour = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;

  //
  // Draw control line to heaven

  LegacyVector3 controlUp(0, 50.0f + (m_id.GetUniqueId() % 50), 0);

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glShadeModel(GL_SMOOTH);
  glDepthMask(false);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laser.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  float w = (lightPos - g_app->m_camera->GetPos()).Mag() * 0.002f;
  w = std::max(0.5f, w);

  for (int i = 0; i < 10; ++i)
  {
    LegacyVector3 thisUp1 = LegacyVector3(0, -20, 0) + controlUp * static_cast<float>(i);
    LegacyVector3 thisUp2 = LegacyVector3(0, -20, 0) + controlUp * static_cast<float>(i + 1);

    int alpha = 255 - 255 * static_cast<float>(i) / 10.0f;
    int alpha2 = 255 - 255 * static_cast<float>(i + 1) / 10.0f;

    alpha *= fabs(sinf(g_gameTime * 2 + static_cast<float>(i) / 5.0f));
    alpha2 *= fabs(sinf(g_gameTime * 2 + static_cast<float>(i + 1) / 5.0f));

    alpha *= s_distanceScale;
    alpha2 *= s_distanceScale;

    float y = static_cast<float>(i) / 10.0f;
    float h = 1.0f / 10.0f;

    glBegin(GL_QUADS);
    glColor4ub(colour.r, colour.g, colour.b, alpha);

    glTexCoord2f(y, 0);
    glVertex3fv((lightPos - camR * w + thisUp1).GetData());
    glTexCoord2f(y, 1);
    glVertex3fv((lightPos + camR * w + thisUp1).GetData());

    glColor4ub(colour.r, colour.g, colour.b, alpha2);

    glTexCoord2f(y + h, 1);
    glVertex3fv((lightPos + camR * w + thisUp2).GetData());
    glTexCoord2f(y + h, 0);
    glVertex3fv((lightPos - camR * w + thisUp2).GetData());
    glEnd();
  }

  glDisable(GL_TEXTURE_2D);

  glDepthMask(true);
  glShadeModel(GL_FLAT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);

  //
  // Draw our signal flash

  int lastSeqId = g_app->m_networkClient->m_lastValidSequenceIdFromServer;

  if ((m_id.GetTeamId() != 255 && (lastSeqId % 10) / 2 == m_id.GetTeamId()) || m_beingReprogrammed[lastSeqId % 3])
  {
    Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_lightPos->GetWorldMatrix(rootMat);
    LegacyVector3 lightPos = worldMat.pos;

    float signalSize = m_ownership / 5.0f;

    glColor4ub(colour.r, colour.g, colour.b, 255);

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

void ControlTower::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  char* word = _in->GetNextToken();
  m_controlBuildingId = atoi(word);
}

int ControlTower::GetBuildingLink() { return m_controlBuildingId; }

void ControlTower::SetBuildingLink(int _buildingId) { m_controlBuildingId = _buildingId; }
