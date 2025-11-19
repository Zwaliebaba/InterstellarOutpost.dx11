#include "pch.h"
#include "generichub.h"
#include "GameApp.h"
#include "camera.h"
#include "controltower.h"
#include "entity_grid.h"
#include "global_world.h"
#include "location.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"
#include "text_stream_readers.h"

DynamicBase::DynamicBase(BuildingType _type)
  : Building(_type),
    m_buildingLink(-1) { strcpy(m_shapeName, "none"); }

void DynamicBase::Initialize(Building* _template)
{
  Building::Initialize(_template);
  m_buildingLink = dynamic_cast<DynamicBase*>(_template)->m_buildingLink;
  SetShapeName(dynamic_cast<DynamicBase*>(_template)->m_shapeName);
}

void DynamicBase::Render(float _predictionTime)
{
  if (m_shape)
    Building::Render(_predictionTime);
}

bool DynamicBase::Advance() { return Building::Advance(); }

void DynamicBase::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);
  m_buildingLink = atoi(_in->GetNextToken());
  SetShapeName(_in->GetNextToken());
}

int DynamicBase::GetBuildingLink() { return m_buildingLink; }

void DynamicBase::SetBuildingLink(int _buildingId)
{
  m_buildingLink = _buildingId;
  //m_powerLink = _buildingId;
}

void DynamicBase::SetShapeName(char* _shapeName)
{
  strcpy(m_shapeName, _shapeName);

  if (strcmp(m_shapeName, "none") != 0)
  {
    SetShape(Resource::GetShape(m_shapeName));

    Matrix34 mat(m_front, m_up, m_pos);

    m_centerPos = m_shape->CalculateCenter(mat);
    m_radius = m_shape->CalculateRadius(mat, m_centerPos);
  }
}

DynamicHub::DynamicHub()
  : DynamicBase(BuildingType::TypeDynamicHub),
    m_currentScore(0),
    m_requiredScore(0),
    m_minActiveLinks(0),
    m_enabled(false),
    m_reprogrammed(false),
    m_numLinks(-1),
    m_activeLinks(0)
{
}

void DynamicHub::Initialize(Building* _template)
{
  DynamicBase::Initialize(_template);
  auto hubCopy = static_cast<DynamicHub*>(_template);
  m_requiredScore = hubCopy->m_requiredScore;
  m_currentScore = hubCopy->m_currentScore;
  m_minActiveLinks = hubCopy->m_minActiveLinks;
}

void DynamicHub::ReprogramComplete()
{
  m_reprogrammed = true;
  g_app->m_soundSystem->TriggerBuildingEvent(this, "Enable");
}

bool DynamicHub::Advance()
{
  if (!m_reprogrammed || m_numLinks == -1)
  {
    m_numLinks = 0;
    //
    // Check to see if our control tower has been captured.
    // This can happen if a user captures the control tower, exits the level and saves,
    // then returns to the level.  The tower is captured and cannot be changed, but
    // the m_enabled state of this building has been lost.

    bool towerFound = false;
    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings[i];
        if (building && building->m_buildingType == BuildingType::TypeControlTower)
        {
          auto tower = static_cast<ControlTower*>(building);
          if (tower->GetBuildingLink() == m_id.GetUniqueId())
          {
            towerFound = true;
            if (tower->m_id.GetTeamId() == m_id.GetTeamId())
            {
              m_reprogrammed = true;
              break;
            }
          }
        }

        if (building && building->m_buildingType == BuildingType::TypeDynamicNode)
        {
          auto node = static_cast<DynamicNode*>(building);
          if (node->GetBuildingLink() == m_id.GetUniqueId())
            m_numLinks++;
        }
      }
    }
    if (!towerFound)
      m_reprogrammed = true;
  }

  if (m_reprogrammed)
  {
    if (m_requiredScore > 0 && m_currentScore >= m_requiredScore && m_activeLinks >= m_minActiveLinks)
      m_enabled = true;

    if (m_currentScore >= m_requiredScore)
    {
      if (m_enabled)
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
        if (gb && !gb->m_online)
        {
          gb->m_online = true;
          g_app->m_globalWorld->EvaluateEvents();
        }
      }
      else if (!g_app->m_location->MissionComplete())
      {
        GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
        if (gb && gb->m_online)
        {
          gb->m_online = false;
          g_app->m_globalWorld->EvaluateEvents();
        }
      }
    }
  }

  return DynamicBase::Advance();
}

void DynamicHub::Render(float _predictionTime) { DynamicBase::Render(_predictionTime); }

void DynamicHub::ActivateLink()
{
  m_activeLinks++;
  if (m_activeLinks >= m_numLinks)
    m_enabled = true;
}

void DynamicHub::DeactivateLink()
{
  m_activeLinks--;
  if (m_activeLinks < m_numLinks)
    m_enabled = false;
}

bool DynamicHub::ChangeScore(int _points)
{
  if (m_reprogrammed && m_currentScore < m_requiredScore)
  {
    m_currentScore += _points;
    m_currentScore = std::min(m_currentScore, m_requiredScore);
    return true;
  }
  return false;
}

void DynamicHub::Read(TextReader* _in, bool _dynamic)
{
  DynamicBase::Read(_in, _dynamic);
  m_requiredScore = atoi(_in->GetNextToken());
  m_currentScore = atoi(_in->GetNextToken());
  m_minActiveLinks = atoi(_in->GetNextToken());
}

const char* DynamicHub::GetObjectiveCounter()
{
  static char result[256];

  if (m_requiredScore > 0)
  {
    float current = m_currentScore;
    float required = m_requiredScore;
    float percentComplete = current / required * 100.0f;
    sprintf(result, "percent complete: %.0f", percentComplete);
  }
  else
    sprintf(result, "");
  return result;
}

int DynamicHub::PointsPerHub() { return m_requiredScore / m_minActiveLinks; }

DynamicNode::DynamicNode()
  : DynamicBase(BuildingType::TypeDynamicNode),
    m_scoreValue(0),
    m_scoreTimer(1.0f),
    m_scoreSupplied(0),
    m_operating(false)
{
}

void DynamicNode::Initialize(Building* _template)
{
  DynamicBase::Initialize(_template);
  auto nodeCopy = static_cast<DynamicNode*>(_template);
  m_scoreValue = nodeCopy->m_scoreValue;
  m_scoreSupplied = nodeCopy->m_scoreSupplied;
}

bool DynamicNode::Advance()
{
  bool friendly = true;
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    if (m_ports[i]->m_occupant.GetTeamId() == 1)
    {
      friendly = false;
      break;
    }
  }

  if (!m_operating)
  {
    if (GetNumPorts() > 0)
    {
      if (GetNumPortsOccupied() == GetNumPorts())
      {
        if (friendly)
        {
          auto hub = static_cast<DynamicHub*>(g_app->m_location->GetBuilding(m_buildingLink));
          if (hub && hub->m_buildingType == BuildingType::TypeDynamicHub)
          {
            m_operating = true;
            hub->ActivateLink();
          }
        }
      }
    }
  }
  else
  {
    if (GetNumPortsOccupied() < GetNumPorts())
    {
      auto hub = static_cast<DynamicHub*>(g_app->m_location->GetBuilding(m_buildingLink));
      if (hub && hub->m_buildingType == BuildingType::TypeDynamicHub)
      {
        m_operating = false;
        hub->DeactivateLink();
      }
    }
    else
    {
      if (m_scoreValue > 0)
      {
        m_scoreTimer -= SERVER_ADVANCE_PERIOD;
        if (m_scoreTimer <= 0.0f)
        {
          m_scoreTimer = 1.0f;
          auto hub = static_cast<DynamicHub*>(g_app->m_location->GetBuilding(m_buildingLink));
          if (hub && hub->m_buildingType == BuildingType::TypeDynamicHub)
          {
            int scoreMod = 1;
            if (!friendly)
              scoreMod = -1;

            if (hub->PointsPerHub() > m_scoreSupplied)
            {
              if (hub->ChangeScore(m_scoreValue * scoreMod))
                m_scoreSupplied += m_scoreValue * scoreMod;
            }
          }
        }
      }
    }
  }
  return DynamicBase::Advance();
}

void DynamicNode::RenderPorts() { Building::RenderPorts(); }

void DynamicNode::Render(float _predictionTime)
{
  glShadeModel(GL_SMOOTH);
  DynamicBase::Render(_predictionTime);
  glShadeModel(GL_FLAT);
}

void DynamicNode::RenderAlphas(float _predictionTime) { DynamicBase::RenderAlphas(_predictionTime); }

void DynamicNode::ReprogramComplete()
{
  if (GetNumPorts() == 0)
  {
    auto hub = static_cast<DynamicHub*>(g_app->m_location->GetBuilding(m_buildingLink));
    if (hub && hub->m_buildingType == BuildingType::TypeDynamicHub)
    {
      m_operating = true;
      hub->ActivateLink();
    }
  }
}

void DynamicNode::Read(TextReader* _in, bool _dynamic)
{
  DynamicBase::Read(_in, _dynamic);
  m_scoreValue = atoi(_in->GetNextToken());
  m_scoreSupplied = atoi(_in->GetNextToken());
}
