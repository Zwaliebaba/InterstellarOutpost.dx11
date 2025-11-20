#include "pch.h"
#include "safearea.h"
#include "GameApp.h"
#include "entity_grid.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "location.h"
#include "team.h"
#include "text_stream_readers.h"

SafeArea::SafeArea()
  : Building(BuildingType::TypeSafeArea),
    m_size(50.0f),
    m_entitiesRequired(0),
    m_entityTypeRequired(EntityType::TypeDarwinian),
    m_recountTimer(0.0f),
    m_entitiesCounted(0) {  }

void SafeArea::Initialize(Building* _template)
{
  Building::Initialize(_template);

  m_size = static_cast<SafeArea*>(_template)->m_size;
  m_entitiesRequired = static_cast<SafeArea*>(_template)->m_entitiesRequired;
  m_entityTypeRequired = static_cast<SafeArea*>(_template)->m_entityTypeRequired;

  m_radius = m_size;
  m_centerPos = m_pos + LegacyVector3(0, m_radius / 2, 0);
}

bool SafeArea::Advance()
{
  //
  // Is the world awake yet ?

  if (!g_app->m_location)
    return false;
  if (!g_app->m_location->m_teams)
    return false;
  if (g_app->m_location->m_teams[m_id.GetTeamId()].m_teamType == Team::TeamTypeUnused)
    return false;

  if (GetHighResTime() > m_recountTimer)
  {
    int numFound;
    WorldObjectId* ids = g_app->m_location->m_entityGrid->GetFriends(m_pos.x, m_pos.z, m_size, &numFound, m_id.GetTeamId());
    m_entitiesCounted = 0;

    for (int i = 0; i < numFound; ++i)
    {
      WorldObjectId id = ids[i];
      Entity* entity = g_app->m_location->GetEntity(id);
      if (entity && entity->m_entityType == m_entityTypeRequired && !entity->m_dead) { ++m_entitiesCounted; }
    }

    m_recountTimer = GetHighResTime() + 2.0f;

    if ((m_id.GetTeamId() == 1 && m_entitiesCounted <= m_entitiesRequired) || (m_id.GetTeamId() != 1 && m_entitiesCounted >=
      m_entitiesRequired))
    {
      GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
      if (gb && !gb->m_online)
      {
        gb->m_online = true;
        g_app->m_globalWorld->EvaluateEvents();
      }
    }
  }

  return false;
}

bool SafeArea::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool SafeArea::DoesShapeHit(Shape* _shape, Matrix34 _transform) { return false; }

bool SafeArea::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                          LegacyVector3* _norm)
{
  return false;
}

const char* SafeArea::GetObjectiveCounter()
{
  static char result[256];
  sprintf(result, "%s : %d", Strings::Get("objective_currentcount").c_str(), m_entitiesCounted);
  return result;
}

void SafeArea::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_size = atof(_in->GetNextToken());
  m_entitiesRequired = atoi(_in->GetNextToken());
  m_entityTypeRequired = (EntityType)atoi(_in->GetNextToken());
}
