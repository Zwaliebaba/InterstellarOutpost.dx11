#include "pch.h"
#include "switch.h"
#include "GameApp.h"
#include "building.h"
#include "camera.h"
#include "NetworkClient.h"
#include "global_world.h"
#include "laserfence.h"
#include "location.h"
#include "resource.h"
#include "Script.h"
#include "shape.h"
#include "text_stream_readers.h"

FenceSwitch::FenceSwitch()
  : Building(BuildingType::TypeFenceSwitch),
    m_linkedBuildingId(-1),
    m_linkedBuildingId2(-1),
    m_switchable(false),
    m_timer(20.0f),
    m_connectionLocation(nullptr),
    m_locked(false),
    m_lockable(0),
    m_switchValue(-1)
{
  Building::SetShape(Resource::GetShape("fenceswitch.shp"));
  strcpy(m_script, "none");
}

void FenceSwitch::Initialize(Building* _template)
{
  Building::Initialize(_template);
  DEBUG_ASSERT(_template->m_buildingType == BuildingType::TypeFenceSwitch);
  m_linkedBuildingId = dynamic_cast<FenceSwitch*>(_template)->m_linkedBuildingId;
  m_linkedBuildingId2 = dynamic_cast<FenceSwitch*>(_template)->m_linkedBuildingId2;
  m_switchValue = static_cast<FenceSwitch*>(_template)->m_switchValue;
  m_lockable = static_cast<FenceSwitch*>(_template)->m_lockable;
  m_locked = static_cast<FenceSwitch*>(_template)->m_locked;
  strcpy(m_script, static_cast<FenceSwitch*>(_template)->m_script);
}

// *** Advance
bool FenceSwitch::Advance()
{
  if (m_timer > 0.0f)
  {
    m_timer -= SERVER_ADVANCE_PERIOD;
    return Building::Advance();
  }

  bool friendly = true;
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    if (m_ports[i]->m_occupant.GetTeamId() == 1)
    {
      friendly = false;
      break;
    }
  }

  if (friendly && GetNumPorts() == GetNumPortsOccupied())
    SetTeamId(2);
  else
    SetTeamId(1);

  Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId);

  bool switched = false;

  if (!m_locked)
  {
    if (b->m_buildingType == BuildingType::TypeLaserFence)
    {
      auto fence = static_cast<LaserFence*>(b);

      if (GetNumPorts() == GetNumPortsOccupied())
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
        if (gb && !gb->m_online)
        {
          if (friendly)
          {
            m_switchable = true;
            if (m_switchValue == m_linkedBuildingId)
            {
              if (!fence->IsEnabled())
              {
                fence->Enable();
                switched = true;
              }

              Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
              if (b2 && b2->m_buildingType == BuildingType::TypeLaserFence)
              {
                auto fence2 = static_cast<LaserFence*>(b2);
                if (fence2->IsEnabled())
                  fence2->Disable();
              }
            }
            else
            {
              if (fence->IsEnabled())
              {
                fence->Disable();
                switched = true;
              }

              Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
              if (b2 && b2->m_buildingType == BuildingType::TypeLaserFence)
              {
                auto fence2 = static_cast<LaserFence*>(b2);
                if (!fence2->IsEnabled())
                  fence2->Enable();
              }
            }
          }
          else
          {
            m_switchable = false;
            m_switchValue = m_linkedBuildingId;

            if (!fence->IsEnabled())
            {
              fence->Enable();
              switched = true;
            }

            Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
            if (b2 && b2->m_buildingType == BuildingType::TypeLaserFence)
            {
              auto fence2 = static_cast<LaserFence*>(b2);
              if (fence2->IsEnabled())
                fence2->Disable();
            }
          }
          gb->m_online = true;
          g_app->m_globalWorld->EvaluateEvents();
        }
      }
      else
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
        if (gb && gb->m_online)
        {
          m_switchValue = m_linkedBuildingId;
          m_switchable = false;

          if (m_linkedBuildingId2 == -1)
          {
            if (fence->IsEnabled())
            {
              fence->Disable();
              switched = true;
            }
          }
          else
          {
            if (!fence->IsEnabled())
            {
              fence->Enable();
              switched = true;
            }

            Building* b2 = g_app->m_location->GetBuilding(m_linkedBuildingId2);
            if (b2 && b2->m_buildingType == BuildingType::TypeLaserFence)
            {
              auto fence2 = static_cast<LaserFence*>(b2);
              if (fence2->IsEnabled())
                fence2->Disable();
            }
          }
          gb->m_online = false;
          g_app->m_globalWorld->EvaluateEvents();
        }
      }
    }
  }

  if (switched)
  {
    if (strstr(m_script, ".txt"))
      g_app->m_script->RunScript(m_script);
    if (m_lockable)
      m_locked = true;
  }

  return Building::Advance();
}

void FenceSwitch::Render(const float predictionTime) { Building::Render(predictionTime); }

void FenceSwitch::RenderAlphas(float predictionTime)
{
  Building* building = g_app->m_location->GetBuilding(m_linkedBuildingId);
  if (building && building->m_buildingType == BuildingType::TypeLaserFence)
  {
    auto fence = static_cast<LaserFence*>(building);
    RenderConnection(fence->GetTopPosition(), m_switchValue == m_linkedBuildingId);
  }

  building = g_app->m_location->GetBuilding(m_linkedBuildingId2);
  if (building && building->m_buildingType == BuildingType::TypeLaserFence)
  {
    auto fence = static_cast<LaserFence*>(building);
    RenderConnection(fence->GetTopPosition(), m_switchValue == m_linkedBuildingId2);
  }
}

void FenceSwitch::RenderConnection(LegacyVector3 _targetPos, const bool _active)
{
  LegacyVector3 ourPos = GetConnectionLocation();
  LegacyVector3 theirPos = _targetPos;

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

  if (_active)
    glColor4f(0.9f, 0.9f, 0.5f, 1.0f);
  else
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

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
}

// *** GetBuildingLink
int FenceSwitch::GetBuildingLink() { return m_linkedBuildingId; }

// *** SetBuildingLink
void FenceSwitch::SetBuildingLink(const int _buildingId)
{
  if (m_linkedBuildingId == -1)
    m_linkedBuildingId = _buildingId;
  else if (m_linkedBuildingId2 == -1)
    m_linkedBuildingId2 = _buildingId;
  else
    m_linkedBuildingId = _buildingId;
}

// *** Read
void FenceSwitch::Read(TextReader* _in, const bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_linkedBuildingId = atoi(_in->GetNextToken());
  m_linkedBuildingId2 = atoi(_in->GetNextToken());
  m_switchValue = atoi(_in->GetNextToken());
  m_lockable = atoi(_in->GetNextToken());
  strcpy(m_script, _in->GetNextToken());
  if (_in->TokenAvailable())
  {
    int locked = atoi(_in->GetNextToken());
    if (locked == 1)
      m_locked = true;
  }

  if (m_switchValue == -1)
    m_switchValue = m_linkedBuildingId;
}

void FenceSwitch::Switch()
{
  if (!m_switchable)
    return;

  if ((m_lockable == 1 && !m_locked) || !m_lockable)
  {
    if (m_switchValue == m_linkedBuildingId)
      m_switchValue = m_linkedBuildingId2;
    else
      m_switchValue = m_linkedBuildingId;

    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    gb->m_online = false;
    g_app->m_globalWorld->EvaluateEvents();
    /*if( strstr( m_script, ".txt" ) )
    {
        g_app->m_script->RunScript( m_script );
    }*/
  }
}

void FenceSwitch::RenderLights()
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

        if (m_switchValue == m_linkedBuildingId)
          glColor3f(0.0f, 1.0f, 0.0f);
        else
          glColor3f(1.0f, 0.0f, 0.0f);

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

LegacyVector3 FenceSwitch::GetConnectionLocation()
{
  if (!m_connectionLocation)
  {
    m_connectionLocation = m_shape->m_rootFragment->LookupMarker("MarkerConnectionLocation");
    DEBUG_ASSERT(m_connectionLocation);
  }

  Matrix34 rootMat(m_front, m_up, m_pos);
  Matrix34 worldPos = m_connectionLocation->GetWorldMatrix(rootMat);
  return worldPos.pos;
}

bool FenceSwitch::IsInView()
{
  if (m_linkedBuildingId != -1)
  {
    LegacyVector3 startPoint = m_pos;
    Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId);
    if (b)
    {
      LegacyVector3 endPoint = b->m_pos;
      LegacyVector3 midPoint = (startPoint + endPoint) / 2.0f;

      float radius = (startPoint - endPoint).Mag() / 2.0f;
      radius += m_radius;

      if (g_app->m_camera->SphereInViewFrustum(midPoint, radius))
        return true;
    }
  }

  if (m_linkedBuildingId2 != -1)
  {
    LegacyVector3 startPoint = m_pos;
    Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId2);
    if (b)
    {
      LegacyVector3 endPoint = b->m_pos;
      LegacyVector3 midPoint = (startPoint + endPoint) / 2.0f;

      float radius = (startPoint - endPoint).Mag() / 2.0f;
      radius += m_radius;

      if (g_app->m_camera->SphereInViewFrustum(midPoint, radius))
        return true;
    }
  }

  return Building::IsInView();
}
