#include "pch.h"
#include "math_utils.h"
#include "resource.h"
#include "shape.h"
#include "GameApp.h"
#include "entity_grid.h"
#include "explosion.h"
#include "location.h"
#include "team.h"
#include "unit.h"

#include "cave.h"

Cave::Cave()
  : Building(BuildingType::TypeCave),
    m_troopType(EntityType::TypeVirii),
    m_unitId(-1),
    m_spawnTimer(0.0f),
    m_dead(false)
{
  Building::SetShape(Resource::GetShape("cave.shp"));

  m_spawnPoint = m_shape->m_rootFragment->LookupMarker("MarkerSpawnPoint");
  DEBUG_ASSERT(m_spawnPoint);
}

bool Cave::Advance()
{
  if (m_dead)
    return true;

  if (m_id.GetTeamId() < 0 || m_id.GetTeamId() >= NUM_TEAMS)
    return false;
  if (g_app->m_location->m_teams[m_id.GetTeamId()].m_teamType == Team::TeamTypeUnused)
    return false;

  m_spawnTimer -= SERVER_ADVANCE_PERIOD;

  if (m_spawnTimer <= 0.0f)
  {
    //
    // Only spawn if the area is sufficiently empty

    Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_spawnPoint->GetWorldMatrix(rootMat);
    LegacyVector3 spawnPoint = worldMat.pos;
    spawnPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(spawnPoint.x, spawnPoint.z);

    int numFound;
    WorldObjectId* objs = g_app->m_location->m_entityGrid->GetFriends(spawnPoint.x, spawnPoint.z, 100.0f, &numFound,
                                                                      m_id.GetTeamId());
    if (numFound < 30)
    {
      g_app->m_location->SpawnEntities(spawnPoint, m_id.GetTeamId(), m_id.GetUniqueId(), m_troopType, 1, g_zeroVector, 0.0f);
    }

    m_spawnTimer = syncfrand(0.5f);
  }

  return false;
}

void Cave::Damage(float _damage)
{
  if (_damage > 80.0f)
  {
    Matrix34 mat(m_front, g_upVector, m_pos);
    g_explosionManager.AddExplosion(m_shape, mat);
    m_dead = true;
  }
}
