#include "pch.h"
#include "generator.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "camera.h"
#include "constructionyard.h"
#include "controltower.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "resource.h"
#include "rocket.h"
#include "shape.h"
#include "soundsystem.h"
#include "text_stream_readers.h"

PowerBuilding::PowerBuilding(BuildingType _type)
  : Building(_type),
    m_powerLink(-1),
    m_powerLocation(nullptr) {}

void PowerBuilding::Initialize(Building* _template)
{
  DEBUG_ASSERT(_template->m_buildingType == BuildingType::TypePowerstation);
  Building::Initialize(_template);
  m_powerLink = dynamic_cast<PowerBuilding*>(_template)->m_powerLink;
}

LegacyVector3 PowerBuilding::GetPowerLocation()
{
  if (!m_powerLocation)
  {
    m_powerLocation = m_shape->m_rootFragment->LookupMarker("MarkerPowerLocation");
    DEBUG_ASSERT(m_powerLocation);
  }

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldPos = m_powerLocation->GetWorldMatrix(rootMat);
  return worldPos.pos;
}

bool PowerBuilding::IsInView()
{
  if (Building* powerLink = g_app->m_location->GetBuilding(m_powerLink))
  {
    LegacyVector3 midPoint = (powerLink->m_centerPos + m_centerPos) / 2.0f;
    float radius = (powerLink->m_centerPos - m_centerPos).Mag() / 2.0f;
    radius += m_radius;

    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

void PowerBuilding::Render(float _predictionTime)
{
  Matrix34 mat(m_front, m_up, m_pos);
  m_shape->Render(_predictionTime, mat);
}

void PowerBuilding::RenderAlphas(float _predictionTime)
{
  Building::RenderAlphas(_predictionTime);

  if (Building* powerLink = g_app->m_location->GetBuilding(m_powerLink))
  {
    //
    // Render the power line itself
    auto powerBuilding = dynamic_cast<PowerBuilding*>(powerLink);

    LegacyVector3 ourPos = GetPowerLocation();
    LegacyVector3 theirPos = powerBuilding->GetPowerLocation();

    LegacyVector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
    LegacyVector3 ourPosRight = camToOurPos ^ (theirPos - ourPos);
    ourPosRight.SetLength(2.0f);

    LegacyVector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
    LegacyVector3 theirPosRight = camToTheirPos ^ (theirPos - ourPos);
    theirPosRight.SetLength(2.0f);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(false);
    glColor4f(0.9f, 0.9f, 0.5f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laser.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

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

    //
    // Render any surges

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));

    float surgeSize = 25.0f;
    glColor4f(0.5f, 0.5f, 1.0f, 1.0f);
    LegacyVector3 camUp = g_app->m_camera->GetUp() * surgeSize;
    LegacyVector3 camRight = g_app->m_camera->GetRight() * surgeSize;
    glBegin(GL_QUADS);
    for (int i = 0; i < m_surges.Size(); ++i)
    {
      float thisSurge = m_surges[i];
      thisSurge += _predictionTime * 2;
      thisSurge = std::clamp(thisSurge, 0.0f, 1.0f);
      LegacyVector3 thisSurgePos = ourPos + (theirPos - ourPos) * thisSurge;
      glTexCoord2i(0, 0);
      glVertex3fv((thisSurgePos - camUp - camRight).GetData());
      glTexCoord2i(1, 0);
      glVertex3fv((thisSurgePos - camUp + camRight).GetData());
      glTexCoord2i(1, 1);
      glVertex3fv((thisSurgePos + camUp + camRight).GetData());
      glTexCoord2i(0, 1);
      glVertex3fv((thisSurgePos + camUp - camRight).GetData());
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
  }
}

bool PowerBuilding::Advance()
{
  for (int i = 0; i < m_surges.Size(); ++i)
  {
    float* thisSurge = m_surges.GetPointer(i);
    *thisSurge += SERVER_ADVANCE_PERIOD * 2;
    if (*thisSurge >= 1.0f)
    {
      m_surges.RemoveData(i);
      --i;

      if (Building* powerLink = g_app->m_location->GetBuilding(m_powerLink))
      {
        auto powerBuilding = dynamic_cast<PowerBuilding*>(powerLink);
        powerBuilding->TriggerSurge(0.0f);
      }
    }
  }
  return Building::Advance();
}

void PowerBuilding::TriggerSurge(float _initValue)
{
  m_surges.PutDataAtStart(_initValue);

  g_app->m_soundSystem->TriggerBuildingEvent(this, "TriggerSurge");
}

void PowerBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);
  m_powerLink = atoi(_in->GetNextToken());
}

int PowerBuilding::GetBuildingLink() { return m_powerLink; }

void PowerBuilding::SetBuildingLink(int _buildingId) { m_powerLink = _buildingId; }

Generator::Generator()
  : PowerBuilding(BuildingType::TypeGenerator),
    m_throughput(0.0f),
    m_timerSync(0.0f),
    m_numThisSecond(0),
    m_enabled(false)
{
  Building::SetShape(Resource::GetShape("generator.shp"));

  m_counter = m_shape->m_rootFragment->LookupMarker("MarkerCounter");
}

void Generator::TriggerSurge(float _initValue)
{
  if (m_enabled)
  {
    PowerBuilding::TriggerSurge(_initValue);
    ++m_numThisSecond;
  }
}

const char* Generator::GetObjectiveCounter()
{
  static char result[256];
  sprintf(result, "%s : %d Gq/s", Strings::Get("objective_output").c_str(), static_cast<int>(m_throughput * 10));
  return result;
}

void Generator::ReprogramComplete()
{
  m_enabled = true;
  g_app->m_soundSystem->TriggerBuildingEvent(this, "Enable");
}

bool Generator::Advance()
{
  if (!m_enabled)
  {
    m_surges.Empty();
    m_throughput = 0.0f;
    m_numThisSecond = 0;

    //
    // Check to see if our control tower has been captured.
    // This can happen if a user captures the control tower, exits the level and saves,
    // then returns to the level.  The tower is captured and cannot be changed, but
    // the m_enabled state of this building has been lost.

    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building && building->m_buildingType == BuildingType::TypeControlTower)
        {
          auto tower = dynamic_cast<ControlTower*>(building);
          if (tower->GetBuildingLink() == m_id.GetUniqueId() && tower->m_id.GetTeamId() == m_id.GetTeamId())
          {
            m_enabled = true;
            break;
          }
        }
      }
    }
  }
  else
  {
    if (GetHighResTime() >= m_timerSync + 1.0f)
    {
      float newAverage = m_numThisSecond;
      m_numThisSecond = 0;
      m_timerSync = GetHighResTime();
      m_throughput = m_throughput * 0.8f + newAverage * 0.2f;
    }

    if (m_throughput > 6.5f)
    {
      GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
      gb->m_online = true;
    }
  }

  return PowerBuilding::Advance();
}

void Generator::Render(float _predictionTime)
{
  PowerBuilding::Render(_predictionTime);

  Matrix34 generatorMat(m_front, g_upVector, m_pos);
  Matrix34 counterMat = m_counter->GetWorldMatrix(generatorMat);

  glColor4f(0.6f, 0.8f, 0.9f, 1.0f);
  g_gameFont.DrawText3D(counterMat.pos, counterMat.f, counterMat.u, 7.0f, "%d", static_cast<int>(m_throughput * 10.0f));
  counterMat.pos += counterMat.f * 0.1f;
  counterMat.pos += (counterMat.f ^ counterMat.u) * 0.2f;
  counterMat.pos += counterMat.u * 0.2f;
  g_gameFont.SetRenderShadow(true);
  glColor4f(0.6f, 0.8f, 0.9f, 0.0f);
  g_gameFont.DrawText3D(counterMat.pos, counterMat.f, counterMat.u, 7.0f, "%d", static_cast<int>(m_throughput * 10.0f));
  g_gameFont.SetRenderShadow(false);
}

Pylon::Pylon()
  : PowerBuilding(BuildingType::TypePylon)
{
  Building::SetShape(Resource::GetShape("pylon.shp"));
}

bool Pylon::Advance() { return PowerBuilding::Advance(); }

PylonStart::PylonStart()
  : PowerBuilding(BuildingType::TypePylonStart),
    m_reqBuildingId(-1)
{
  Building::SetShape(Resource::GetShape("pylon.shp"));
};

void PylonStart::Initialize(Building* _template)
{
  PowerBuilding::Initialize(_template);

  m_reqBuildingId = dynamic_cast<PylonStart*>(_template)->m_reqBuildingId;
}

bool PylonStart::Advance()
{
  //
  // Is the Generator online?

  bool generatorOnline = false;

  int generatorLocationId = g_app->m_globalWorld->GetLocationId("generator");

  for (int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i)
  {
    if (g_app->m_globalWorld->m_buildings.ValidIndex(i))
    {
      GlobalBuilding* gb = g_app->m_globalWorld->m_buildings[i];
      if (gb && gb->m_locationId == generatorLocationId && gb->m_buildingType == BuildingType::TypeGenerator && gb->m_online)
      {
        generatorOnline = true;
        break;
      }
    }
  }

  if (generatorOnline)
  {
    //
    // Is our required building online yet?
    GlobalBuilding* globalBuilding = g_app->m_globalWorld->GetBuilding(m_reqBuildingId, g_app->m_locationId);
    if (globalBuilding && globalBuilding->m_online) { if (syncfrand() > 0.7f) { TriggerSurge(0.0f); } }
  }

  return PowerBuilding::Advance();
}

void PylonStart::Read(TextReader* _in, bool _dynamic)
{
  PowerBuilding::Read(_in, _dynamic);

  m_reqBuildingId = atoi(_in->GetNextToken());
}

PylonEnd::PylonEnd()
  : PowerBuilding(BuildingType::TypePylonEnd)
{
  Building::SetShape(Resource::GetShape("pylon.shp"));
};

void PylonEnd::TriggerSurge(float _initValue)
{
  Building* building = g_app->m_location->GetBuilding(m_powerLink);

  if (building && building->m_buildingType == BuildingType::TypeYard)
  {
    auto yard = dynamic_cast<ConstructionYard*>(building);
    yard->AddPowerSurge();
  }

  if (building && building->m_buildingType == BuildingType::TypeFuelGenerator)
  {
    auto fuel = dynamic_cast<FuelGenerator*>(building);
    fuel->ProvideSurge();
  }
}

SolarPanel::SolarPanel()
  : PowerBuilding(BuildingType::TypeSolarPanel),
    m_operating(false)
{
  Building::SetShape(Resource::GetShape("solarpanel.shp"));

  memset(m_glowMarker, 0, SOLARPANEL_NUMGLOWS * sizeof(ShapeMarker*));

  for (int i = 0; i < SOLARPANEL_NUMGLOWS; ++i)
  {
    char name[64];
    sprintf(name, "MarkerGlow0%d", i + 1);
    m_glowMarker[i] = m_shape->m_rootFragment->LookupMarker(name);
    DEBUG_ASSERT(m_glowMarker[i]);
  }

  for (int i = 0; i < SOLARPANEL_NUMSTATUSMARKERS; ++i)
  {
    char name[64];
    sprintf(name, "MarkerStatus0%d", i + 1);
    m_statusMarkers[i] = m_shape->m_rootFragment->LookupMarker(name);
  }
}

void SolarPanel::Initialize(Building* _template)
{
  _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(_template->m_pos.x, _template->m_pos.z);
  auto right = LegacyVector3(1, 0, 0);
  _template->m_front = right ^ _template->m_up;

  PowerBuilding::Initialize(_template);
}

bool SolarPanel::Advance()
{
  float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

  if (syncfrand(20.0f) <= fractionOccupied) { TriggerSurge(0.0f); }

  if (fractionOccupied > 0.6f)
  {
    if (!m_operating)
      g_app->m_soundSystem->TriggerBuildingEvent(this, "Operate");
    m_operating = true;
  }

  if (fractionOccupied < 0.3f)
  {
    if (m_operating)
      g_app->m_soundSystem->StopAllSounds(m_id, "SolarPanel Operate");
    m_operating = false;
  }

  return PowerBuilding::Advance();
}

void SolarPanel::RenderPorts()
{
  glDisable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    Matrix34 rootMat(m_front, m_up, m_pos);
    DEBUG_ASSERT(m_statusMarkers[i] != nullptr);
    Matrix34 worldMat = m_statusMarkers[i]->GetWorldMatrix(rootMat);

    //
    // Render the status light

    float size = 6.0f;

    LegacyVector3 camR = g_app->m_camera->GetRight() * size;
    LegacyVector3 camU = g_app->m_camera->GetUp() * size;

    LegacyVector3 statusPos = worldMat.pos;

    if (GetPortOccupant(i).IsValid())
      glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
    else
      glColor4f(1.0f, 0.3f, 0.3f, 1.0f);

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
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glDepthMask(true);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}

void SolarPanel::Render(float _predictionTime)
{
  glShadeModel(GL_SMOOTH);
  PowerBuilding::Render(_predictionTime);
  glShadeModel(GL_FLAT);
}

void SolarPanel::RenderAlphas(float _predictionTime)
{
  PowerBuilding::RenderAlphas(_predictionTime);

  float fractionOccupied = static_cast<float>(GetNumPortsOccupied()) / static_cast<float>(GetNumPorts());

  if (fractionOccupied > 0.0f)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    float alphaValue = fabs(sinf(g_gameTime)) * fractionOccupied;

    glColor4f(0.2f, 0.4f, 0.9f, alphaValue);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\glow.bmp"));
    glDepthMask(false);
    glDisable(GL_CULL_FACE);

    for (int i = 0; i < SOLARPANEL_NUMGLOWS; ++i)
    {
      float glowHeight = 40.0f;
      float glowWidth = 60.0f;
      Matrix34 thisGlow = m_glowMarker[i]->GetWorldMatrix(mat);

      glBegin(GL_QUADS);
      glTexCoord2i(0, 0);
      glVertex3fv((thisGlow.pos - thisGlow.r * glowHeight + thisGlow.f * glowWidth).GetData());
      glTexCoord2i(0, 1);
      glVertex3fv((thisGlow.pos + thisGlow.r * glowHeight + thisGlow.f * glowWidth).GetData());
      glTexCoord2i(1, 1);
      glVertex3fv((thisGlow.pos + thisGlow.r * glowHeight - thisGlow.f * glowWidth).GetData());
      glTexCoord2i(1, 0);
      glVertex3fv((thisGlow.pos - thisGlow.r * glowHeight - thisGlow.f * glowWidth).GetData());
      glEnd();
    }

    glEnable(GL_CULL_FACE);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
  }
}
