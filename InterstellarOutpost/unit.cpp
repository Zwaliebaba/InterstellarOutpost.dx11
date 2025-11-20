#include "pch.h"
#include "unit.h"
#include "GameApp.h"
#include "RoutingSystem.h"
#include "camera.h"
#include "entity_grid.h"
#include "lasertrooper.h"
#include "level_file.h"
#include "location.h"
#include "math_utils.h"
#include "profiler.h"
#include "team.h"
#include "worldobject.h"

Unit::Unit(EntityType troopType, int teamId, int unitId, int numEntities, const LegacyVector3& _pos)
  : m_wayPoint(0, 0, 0),
    m_routeId(-1),
    m_routeWayPointId(-1),
    m_teamId(teamId),
    m_unitId(unitId),
    m_troopType(troopType),
    m_centerPos(_pos),
    m_vel(0, 0, 0),
    m_radius(0.0f),
    m_targetDir(1, 0, 0),
    m_attackAccumulator(0.0f),
    m_accumulatedCenter(0, 0, 0),
    m_accumulatedRadiusSquared(0.0f),
    m_numAccumulated(0)
{
  m_entities.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
  m_entities.SetStepSize(100);
  m_entities.SetSize(numEntities);
}

Unit::~Unit()
{
  Team* myTeam = &g_app->m_location->m_teams[m_teamId];
  if (myTeam->m_currentUnitId == m_unitId)
    myTeam->m_currentUnitId = -1;
}

void Unit::Begin() {}

Entity* Unit::NewEntity(int* _index)
{
  Entity* entity = Entity::NewEntity(m_troopType);
  *_index = m_entities.PutData(entity);
  return entity;
}

int Unit::AddEntity(Entity* _entity) { return m_entities.PutData(_entity); }

// Removes an entity from the unit's array of entities. Also removes the
// entity from the EntityGrid and deletes the entity.
// _posX and _posZ specify the position that the entity last registered
// with the EntityGrid.
void Unit::RemoveEntity(int _index, float _posX, float _posZ)
{
  if (m_entities.ValidIndex(_index))
  {
    Entity* entity = m_entities[_index];

    WorldObjectId myId(m_teamId, m_unitId, _index, entity->m_id.GetUniqueId());

    g_app->m_location->m_entityGrid->RemoveObject(myId, _posX, _posZ, entity->m_radius);

    m_entities.MarkNotUsed(_index);
    delete entity;
  }
}

void Unit::AdvanceEntities(int _slice)
{
  int startIndex, endIndex;
  m_entities.GetNextSliceBounds(_slice, &startIndex, &endIndex);

  for (int i = startIndex; i <= endIndex; i++)
  {
    if (m_entities.ValidIndex(i))
    {
      Entity* s = m_entities[i];

      if (s->m_enabled)
      {
        LegacyVector3 oldPos(s->m_pos);

        START_PROFILE(g_app->m_profiler, Entity::GetTypeName( s->m_entityType ));
        bool amIdead = s->Advance(this);
        END_PROFILE(g_app->m_profiler, Entity::GetTypeName( s->m_entityType ));

        if (amIdead)
          RemoveEntity(i, oldPos.x, oldPos.z);
        else
        {
          WorldObjectId myId(m_teamId, m_unitId, i, s->m_id.GetUniqueId());
          g_app->m_location->m_entityGrid->UpdateObject(myId, oldPos.x, oldPos.z, s->m_pos.x, s->m_pos.z, s->m_radius);
        }
      }
    }
  }
}

bool Unit::IsInView() { return (g_app->m_camera->SphereInViewFrustum(m_centerPos, m_radius)); }

void Unit::Render(float _predictionTime)
{
  // Render all the entities that are up-to-date with server advances
  int lastUpdated = m_entities.GetLastUpdated();
  for (int i = 0; i <= lastUpdated; i++)
  {
    if (m_entities.ValidIndex(i))
    {
      Entity* entity = m_entities[i];
      entity->Render(_predictionTime);
    }
  }

  // Render all the entities that are one step out-of-date with server advances
  int size = m_entities.Size();
  _predictionTime += SERVER_ADVANCE_PERIOD;
  for (int i = lastUpdated + 1; i < size; i++)
  {
    if (m_entities.ValidIndex(i))
    {
      Entity* entity = m_entities[i];
      entity->Render(_predictionTime);
    }
  }

  glEnable(GL_CULL_FACE);
}

bool Unit::Advance(int _slice)
{
  //
  // Maintain our center and radius values

  LegacyVector3 oldPos = m_centerPos;
  m_centerPos = m_accumulatedCenter;
  if (m_numAccumulated != 0)
    m_centerPos /= m_numAccumulated;
  m_vel = (m_centerPos - oldPos) / SERVER_ADVANCE_PERIOD;
  m_radius = sqrtf(m_accumulatedRadiusSquared);

  if (m_entities.NumUsed() == 0)
  {
    m_radius = 0.0f;
    return true;
  }

  m_accumulatedCenter.Zero();
  m_accumulatedRadiusSquared = 0.0f;
  m_numAccumulated = 0;

  if (m_routeId != -1)
    FollowRoute();

  bool waypointChanged = false;
  if (waypointChanged)
    RecalculateOffsets();

  float leadDistance = 1.0f;

  if (m_troopType == EntityType::TypeLaserTroop)
  {
    for (int i = 0; i < m_entities.Size(); i++)
    {
      if (m_entities.ValidIndex(i))
      {
        auto l = static_cast<LaserTrooper*>(m_entities[i]);

        if ((l->m_pos - l->m_targetPos).Mag() < leadDistance / 5.0f)
        {
          LegacyVector3 pos = l->m_pos;
          //                    LegacyVector3 targetPos = m_wayPoint;
          //                    targetPos += GetFormationOffset( FormationRectangle, l->m_unitIndex );
          //                    targetPos = l->PushFromObstructions( targetPos );
          //                    //targetPos = l->PushFromEachOther( targetPos );
          //                    l->m_unitTargetPos = targetPos;

          LegacyVector3 targetPos = l->m_unitTargetPos;
          LegacyVector3 desiredDirection = (targetPos - pos).Normalize();
          float distance = (targetPos - pos).Mag();
          float amountToMove = leadDistance;
          if (amountToMove > distance)
            amountToMove = distance;
          pos += desiredDirection * amountToMove;
          pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
          pos = l->PushFromObstructions(pos);
          //pos = l->PushFromEachOther( pos );

          l->m_targetPos = pos;
        }
      }
    }
  }

  return false;
}

int Unit::NumEntities() { return m_entities.NumUsed(); }

int Unit::NumAliveEntities()
{
  int result = 0;

  for (int i = 0; i < m_entities.Size(); ++i)
  {
    if (m_entities.ValidIndex(i))
    {
      Entity* entity = m_entities[i];
      if (!entity->m_dead)
        ++result;
    }
  }

  return result;
}

void Unit::Attack(LegacyVector3 pos, bool _withGrenade)
{
  //
  // Deal with grenades

  if (_withGrenade)
  {
    float nearest = 9999.9f;
    Entity* nearestEnt = nullptr;

    //
    // Find the entity nearest to the target that has a grenade

    for (int i = 0; i < m_entities.Size(); ++i)
    {
      if (m_entities.ValidIndex(i))
      {
        Entity* ent = m_entities[i];
        if (!ent->m_dead)
        {
          float distance = (ent->m_pos - pos).Mag();
          if (distance < nearest)
          {
            nearest = distance;
            nearestEnt = ent;
          }
        }
      }
    }

    if (nearestEnt)
      g_app->m_location->ThrowWeapon(nearestEnt->m_pos, pos, EffectType::EffectThrowableGrenade, m_teamId);
  }

  //
  // Build a list of entities that can attack now

  LList<int> canAttack;
  for (int i = 0; i < m_entities.Size(); ++i)
  {
    if (m_entities.ValidIndex(i))
    {
      Entity* ent = m_entities[i];
      if (ent->m_enabled && !ent->m_dead && ent->m_reloading == 0.0f)
        canAttack.PutData(i);
    }
  }

  if (canAttack.Size() > 0)
  {
    //
    // Decide the maximum number of entities
    // that can attack now without pauses appearing in fire rate

    float reloadTime = EntityBlueprint::GetStat(m_troopType, Entity::StatRate);
    float timeToWait = reloadTime / static_cast<float>(NumEntities());
    m_attackAccumulator += (SERVER_ADVANCE_PERIOD / timeToWait);

    //
    // Pick guys randomly to attack

    while (canAttack.Size() > 0 && m_attackAccumulator >= 1.0f)
    {
      m_attackAccumulator -= 1.0f;
      int randomIndex = syncfrand(canAttack.Size());
      int entityIndex = canAttack[randomIndex];
      canAttack.RemoveData(randomIndex);
      Entity* ent = m_entities[entityIndex];
      ent->Attack(pos);
    }
  }
}

void Unit::UpdateEntityPosition(LegacyVector3 pos, float _radius)
{
  m_accumulatedCenter += pos;
  ++m_numAccumulated;

  float distanceFromCenter = (pos - m_centerPos).Mag();
  distanceFromCenter += _radius;
  float distanceFromCenterSquared = distanceFromCenter * distanceFromCenter;

  if (distanceFromCenterSquared > m_accumulatedRadiusSquared)
    m_accumulatedRadiusSquared = distanceFromCenterSquared;
}

LegacyVector3 Unit::GetWayPoint() { return m_wayPoint; }

void Unit::SetWayPoint(const LegacyVector3& _pos) { m_wayPoint = _pos; }

LegacyVector3 Unit::GetFormationOffset(int _formation, int _index)
{
  static float* s_offsets = nullptr;
  constexpr int numOffsets = 100;
  constexpr float spacedOut = 4.0f;

  if (_index == -1)
    return g_zeroVector;

  // Generate some noise in our formation
  if (!s_offsets)
  {
    s_offsets = NEW float[numOffsets];
    for (int i = 0; i < numOffsets; i++)
      s_offsets[i] = syncfrand(spacedOut / 4.0f);
  }

  switch (_formation)
  {
    case FormationRectangle:
    {
      int rowLen = sqrtf(NumEntities());

      float x = _index % rowLen;
      x -= rowLen / 2.0f;
      x *= spacedOut;

      float z = _index / rowLen;
      z -= rowLen / 2.0f;
      z *= spacedOut;

      x += s_offsets[_index % numOffsets];
      z += s_offsets[_index % numOffsets];

      return LegacyVector3(z, 0, x);
    }

    case FormationAirstrike:
    {
      int rowLen = 4;

      float x = _index % rowLen;
      x -= rowLen / 2.0f;
      x *= spacedOut * 6.0f;

      float y = _index / rowLen;
      y -= rowLen / 2.0f;
      y *= spacedOut * 3.0f;

      float z = _index / rowLen;
      z -= rowLen / 2.0f;
      z *= spacedOut * 6.0f;

      x += s_offsets[_index % numOffsets];
      y += s_offsets[_index % numOffsets];
      z += s_offsets[_index % numOffsets];

      return LegacyVector3(x, -y, z);
    }
  }

  return g_zeroVector;
}

LegacyVector3 Unit::GetOffset(int _formation, int _index)
{
  LegacyVector3 formationOffset = GetFormationOffset(_formation, _index);
  LegacyVector3 finalPos = m_wayPoint + formationOffset;

  if (finalPos.Mag() == 0.0f)
    return finalPos;
  return (finalPos - m_wayPoint);
}

void Unit::RecalculateOffsets()
{
  int offset = 0;

  for (int i = 0; i < m_entities.Size(); ++i)
  {
    if (m_entities.ValidIndex(i))
    {
      Entity* ent = m_entities[i];
      if (!ent->m_dead)
      {
        ent->m_formationIndex = offset;
        ++offset;
      }
      else
        ent->m_formationIndex = -1;
    }
  }

  if (m_troopType == EntityType::TypeLaserTroop)
  {
    for (int i = 0; i < m_entities.Size(); i++)
    {
      if (m_entities.ValidIndex(i))
      {
        auto l = static_cast<LaserTrooper*>(m_entities[i]);
        LegacyVector3 pos = l->m_pos;
        LegacyVector3 targetPos = m_wayPoint;
        targetPos += GetFormationOffset(FormationRectangle, l->m_id.GetIndex());
        targetPos = l->PushFromObstructions(targetPos);
        //targetPos = l->PushFromEachOther( targetPos );
        l->m_unitTargetPos = targetPos;
      }
    }
  }
}

void Unit::FollowRoute()
{
  DEBUG_ASSERT(m_routeId != -1);
  Route* route = g_app->m_location->m_levelFile->GetRoute(m_routeId);
  DEBUG_ASSERT(route);

  if (m_routeWayPointId == -1)
    m_routeWayPointId = 0;

  WayPoint* waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
  m_wayPoint = waypoint->GetPos();
  LegacyVector3 targetVect = m_wayPoint - m_centerPos;

  if (waypoint->m_type != WayPoint::TypeBuilding && targetVect.Mag() < 10.0f)
  {
    m_routeWayPointId++;
    if (m_routeWayPointId >= route->m_wayPoints.Size())
    {
      m_routeWayPointId = -1;
      m_routeId = -1;
    }
  }

  //
  // If its a building instead of a 3D pos, this unit will never
  // get to the next waypoint.  A new unit is created when the unit
  // enters the teleport, and taht new unit will automatically
  // continue towards the next waypoint instead.
  //
}

Entity* Unit::RayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir)
{
  for (unsigned int i = 0; i < m_entities.Size(); ++i)
    if (m_entities.ValidIndex(i))
      if (m_entities[i]->RayHit(_rayStart, _rayDir))
        return m_entities[i];

  return nullptr;
}

void Unit::DirectControl(const TeamControls& _teamControls) {}
