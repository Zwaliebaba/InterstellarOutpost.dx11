#include "pch.h"

#include "virii.h"
#include "GameApp.h"
#include "camera.h"
#include "egg.h"
#include "entity_grid.h"
#include "globals.h"
#include "location.h"
#include "math_utils.h"
#include "ParticleSystem.h"
#include "preferences.h"
#include "profiler.h"
#include "Renderer.h"
#include "resource.h"
#include "soundsystem.h"
#include "team.h"

ViriiUnit::ViriiUnit(int teamId, int unitId, int numEntities, const LegacyVector3& _pos)
  : Unit(EntityType::TypeVirii, teamId, unitId, numEntities, _pos),
    m_enemiesFound(false),
    m_cameraClose(false) {}

bool ViriiUnit::Advance(int _slice)
{
  float searchRadius = m_radius + VIRII_MAXSEARCHRANGE;

  m_enemiesFound = g_app->m_location->m_entityGrid->AreEnemiesPresent(m_centerPos.x, m_centerPos.z, searchRadius, m_teamId);

  return Unit::Advance(_slice);
}

void ViriiUnit::Render(float _predictionTime)
{
  //
  // Render Red Virii shapes

  float nearPlaneStart = g_app->m_renderer->GetNearPlane();
  g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05f, g_app->m_renderer->GetFarPlane());

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites\\viriifull.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(false);
  glDisable(GL_CULL_FACE);
  glBegin(GL_QUADS);

  //
  // Render high detail?

  int entityDetail = g_prefsManager->GetInt("RenderEntityDetail");
  int viriiDetail = 1;
  if (entityDetail == 3)
    viriiDetail = 4;
  else
  {
    float rangeToCam = (m_centerPos - g_app->m_camera->GetPos()).Mag();
    if (entityDetail == 1 && rangeToCam > 1000.0f)
      viriiDetail = 2;
    else if (entityDetail == 2 && rangeToCam > 1000.0f)
      viriiDetail = 3;
    else if (entityDetail == 2 && rangeToCam > 500.0f)
      viriiDetail = 2;
    //else if ( entityDetail == 3 && rangeToCam > 1000.0f )        viriiDetail = 4;
    //else if ( entityDetail == 3 && rangeToCam > 600.0f )         viriiDetail = 3;
    //else if ( entityDetail == 3 && rangeToCam > 300.0f )         viriiDetail = 2;
  }

  //
  // Render all the entities that are up-to-date with server advances

  int lastUpdated = m_entities.GetLastUpdated();
  for (int i = 0; i <= lastUpdated; i++)
  {
    if (m_entities.ValidIndex(i))
    {
      auto virii = static_cast<Virii*>(m_entities[i]);
      virii->Render(_predictionTime, m_teamId, viriiDetail);
    }
  }

  //
  // Render all the entities that are one step out-of-date with server advances

  int size = m_entities.Size();
  _predictionTime += SERVER_ADVANCE_PERIOD;
  for (int i = lastUpdated + 1; i < size; i++)
  {
    if (m_entities.ValidIndex(i))
    {
      auto virii = static_cast<Virii*>(m_entities[i]);
      virii->Render(_predictionTime, m_teamId, viriiDetail);
    }
  }

  glEnd();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glDepthMask(true);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  g_app->m_camera->SetupProjectionMatrix(nearPlaneStart, g_app->m_renderer->GetFarPlane());
}

Virii::Virii()
  : Entity(EntityType::TypeVirii),
    m_state(StateIdle),
    m_hoverHeight(0.1f),
    m_retargetTimer(-1),
    m_spiritId(-1),
    m_historyTimer(0.0f),
    m_prevPosTimer(0.0f) { m_retargetTimer = syncfrand(2.0f); }

Virii::~Virii() { m_positionHistory.EmptyAndDelete(); }

bool Virii::Advance(Unit* _unit)
{
  m_prevPosTimer -= SERVER_ADVANCE_PERIOD;

  if (m_prevPosTimer <= 0.0f)
  {
    m_prevPos = m_pos;
    m_prevPosTimer = 5.0f;
  }

  //
  // Take damage if we're in the water
  // Actually, don't
  // if( m_pos.y < 0.0f ) ChangeHealth( -1 );

  bool amIDead = Entity::Advance(_unit);

  if (m_dead)
  {
    m_vel.Zero();

    if (m_spiritId != -1)
    {
      if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
      {
        Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
        if (spirit && spirit->m_state == Spirit::StateAttached)
          spirit->CollectorDrops();
      }
      m_spiritId = -1;
    }
  }

  if (!m_onGround)
  {
    m_vel.y += -10.0f * SERVER_ADVANCE_PERIOD;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    float groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 2.0f;
    if (m_pos.y <= groundLevel)
    {
      m_onGround = true;
      m_vel = g_zeroVector;
    }
  }

  if (!m_dead)
  {
    switch (m_state)
    {
      case StateIdle:
        amIDead = AdvanceIdle();
        break;
      case StateAttacking:
        amIDead = AdvanceAttacking();
        break;
      case StateToSpirit:
        amIDead = AdvanceToSpirit();
        break;
      case StateToEgg:
        amIDead = AdvanceToEgg();
        break;
    }

    if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
    {
      Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
      if (spirit && spirit->m_state == Spirit::StateAttached)
      {
        spirit->m_pos = m_pos;
        spirit->m_pos.y += 2.0f;
        spirit->m_vel = m_vel;
      }
    }

    // Next 5 lines by Andrew. Purpose - to prevent virii grinding themselves against
    // the side of obstructions.
    //        LegacyVector3 newPos(PushFromObstructions( m_pos ));
    //		if (newPos.x != m_pos.x || newPos.z != m_pos.z)
    //		{
    //			m_pos = newPos;
    //			if (m_retargetTimer > 0.05f) m_retargetTimer = 0.1f;
    //		}
  }

  if (m_positionHistory.Size() > 0)
  {
    bool recorded = false;

    if (m_state != StateIdle)
    {
      m_historyTimer -= SERVER_ADVANCE_PERIOD;
      if (m_historyTimer <= 0.0f)
      {
        m_historyTimer = 0.5f;
        RecordHistoryPosition(true);
        recorded = true;
      }
    }

    if (!recorded)
    {
      float lastRecordedHeight = m_positionHistory[0]->m_pos.y;
      if (fabs(m_pos.y - lastRecordedHeight) > 3.0f)
        RecordHistoryPosition(false);
    }
  }
  else
    RecordHistoryPosition(true);

  float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
  float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
  if (m_pos.x < 0.0f)
    m_pos.x = 0.0f;
  if (m_pos.z < 0.0f)
    m_pos.z = 0.0f;
  if (m_pos.x >= worldSizeX)
    m_pos.x = worldSizeX;
  if (m_pos.z >= worldSizeZ)
    m_pos.z = worldSizeZ;

  return amIDead;
}

void Virii::RecordHistoryPosition(bool _required)
{
  START_PROFILE(g_app->m_profiler, "RecordHistory");

  LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
  LegacyVector3 prevPos;
  if (m_positionHistory.Size() > 0)
    prevPos = m_positionHistory[0]->m_pos;

  auto history = new ViriiHistory();
  history->m_pos = m_pos;
  history->m_right = (m_pos - prevPos) ^ landNormal;
  history->m_right.Normalize();
  history->m_distance = 0.0f;
  history->m_required = _required;

  if (m_positionHistory.Size() > 0)
  {
    history->m_distance = (m_pos - m_positionHistory[0]->m_pos).Mag();
    history->m_glowDiff = (m_pos - m_positionHistory[0]->m_pos);
    history->m_glowDiff.SetLength(10.0f);
  }

  m_positionHistory.PutDataAtStart(history);

  float totalDistance = 0.0f;
  int removeFrom = -1;
  int entityDetail = g_prefsManager->GetInt("RenderEntityDetail", 1);
  float tailLength = 175.0f;          // - (entityDetail-1) * 50.0f;
  for (int i = 0; i < m_positionHistory.Size(); ++i)
  {
    ViriiHistory* history = m_positionHistory[i];
    totalDistance += history->m_distance;

    if (totalDistance > tailLength)
    {
      removeFrom = i;
      break;
    }
  }

  if (removeFrom != -1)
  {
    while (m_positionHistory.ValidIndex(removeFrom))
    {
      ViriiHistory* history = m_positionHistory[removeFrom];
      m_positionHistory.RemoveData(removeFrom);
      delete history;
    }
  }

  END_PROFILE(g_app->m_profiler, "RecordHistory");
}

bool Virii::AdvanceToTargetPos(const LegacyVector3& _pos)
{
  START_PROFILE(g_app->m_profiler, "AdvanceToTargetPos");

  LegacyVector3 oldPos = m_pos;

  LegacyVector3 distance = _pos - m_pos;
  m_vel = distance;
  m_vel.Normalize();
  m_front = m_vel;
  m_vel *= m_stats[StatSpeed];
  distance.y = 0.0f;

  LegacyVector3 nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
  nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z) + m_hoverHeight;
  float currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  float nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);

  //
  // Slow us down if we're going up hill
  // Speed up if going down hill

  float factor = 1.0f - (currentHeight - nextHeight) / -5.0f;
  if (factor < 0.1f)
    factor = 0.1f;
  if (factor > 2.0f)
    factor = 2.0f;
  m_vel *= factor;
  nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
  nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z) + m_hoverHeight;

  //
  // Are we there?

  bool arrived = false;
  if (distance.MagSquared() < m_vel.MagSquared() * SERVER_ADVANCE_PERIOD * SERVER_ADVANCE_PERIOD)
  {
    nextPos = _pos;
    arrived = true;
  }

  m_pos = nextPos;
  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

  END_PROFILE(g_app->m_profiler, "AdvanceToTargetPos");

  return arrived;
}

bool Virii::AdvanceIdle()
{
  START_PROFILE(g_app->m_profiler, "AdvanceIdle");

  m_retargetTimer -= SERVER_ADVANCE_PERIOD;
  bool foundTarget = false;

  if (m_retargetTimer <= 0.0f)
  {
    m_retargetTimer = 1.0f;

    if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
      foundTarget = SearchForEggs();

    if (!foundTarget)
      foundTarget = SearchForEnemies();
    if (!foundTarget)
      foundTarget = SearchForSpirits();
    if (!foundTarget)
      foundTarget = SearchForIdleDirection();
  }

  if (m_state == StateIdle)
    AdvanceToTargetPos(m_wayPoint);

  END_PROFILE(g_app->m_profiler, "AdvanceIdle");
  return false;
}

bool Virii::AdvanceAttacking()
{
  START_PROFILE(g_app->m_profiler, "AdvanceAttacking");

  WorldObject* enemy = g_app->m_location->GetEntity(m_enemyId);
  auto entity = static_cast<Entity*>(enemy);

  if (!entity || entity->m_dead)
  {
    m_state = StateIdle;
    END_PROFILE(g_app->m_profiler, "AdvanceAttacking");
    return false;
  }

  //    int enemySpiritId = g_app->m_location->GetSpirit( m_enemyId );
  //    if( enemySpiritId != -1 && entity->m_dead )
  //    {
  //        // Our enemy has died and dropped a spirit
  //        // If we can see an egg, then go for it
  //        if( FindNearbyEgg( enemySpiritId, VIRII_MAXSEARCHRANGE ).IsValid() )
  //        {
  //            m_enemyId.SetInvalid();
  //            m_spiritId = enemySpiritId;
  //            m_state = StateToSpirit;
  //        }
  //        return false;
  //    }

  bool arrived = AdvanceToTargetPos(entity->m_pos);
  if (arrived)
  {
    entity->ChangeHealth(-20);
    for (int i = 0; i < 3; ++i)
    {
      g_app->m_particleSystem->CreateParticle(m_pos, LegacyVector3(syncsfrand(15.0f), syncsfrand(15.0f) + 15.0f, syncsfrand(15.0f)),
                                              Particle::TypeMuzzleFlash);
    }
    g_app->m_soundSystem->TriggerEntityEvent(this, "Attack");
    SearchForEnemies();
  }

  END_PROFILE(g_app->m_profiler, "AdvanceAttacking");
  return false;
}

bool Virii::AdvanceToSpirit()
{
  START_PROFILE(g_app->m_profiler, "AdvanceToSpirit");

  Spirit* s = nullptr;
  if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
    s = g_app->m_location->m_spirits.GetPointer(m_spiritId);

  if (!s || s->m_state == Spirit::StateDeath || s->m_state == Spirit::StateAttached || s->m_state == Spirit::StateInEgg)
  {
    m_spiritId = -1;
    m_state = StateIdle;
    END_PROFILE(g_app->m_profiler, "AdvanceToSpirit");
    return false;
  }

  Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
  LegacyVector3 targetPos = spirit->m_pos;
  targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);
  bool arrived = AdvanceToTargetPos(targetPos);
  if (arrived)
  {
    spirit->CollectorArrives();
    m_state = StateToEgg;
  }

  END_PROFILE(g_app->m_profiler, "AdvanceToSpirit");
  return false;
}

bool Virii::AdvanceToEgg()
{
  START_PROFILE(g_app->m_profiler, "AdvanceToEgg");
  auto theEgg = static_cast<Egg*>(g_app->m_location->GetEntitySafe(m_eggId, EntityType::TypeEgg));

  if (!theEgg || theEgg->m_state == Egg::StateFertilised)
  {
    bool found = SearchForEggs();
    if (!found)
    {
      // We can't find any eggs, so go into holding pattern
      if (g_app->m_location->m_spirits.ValidIndex(m_spiritId))
      {
        Spirit* spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
        if (spirit->m_state == Spirit::StateAttached)
          spirit->CollectorDrops();
        m_spiritId = -1;
      }
      m_state = StateIdle;
      END_PROFILE(g_app->m_profiler, "AdvanceToEgg");
      return false;
    }
  }

  if (!g_app->m_location->m_spirits.ValidIndex(m_spiritId))
  {
    m_spiritId = -1;
    m_state = StateIdle;
    END_PROFILE(g_app->m_profiler, "AdvanceToEgg");
    return false;
  }

  //
  // At this point we MUST have found an egg, otherwise we'd have returned by now

  theEgg = static_cast<Egg*>(g_app->m_location->GetEntitySafe(m_eggId, EntityType::TypeEgg));
  DEBUG_ASSERT(theEgg);

  bool arrived = AdvanceToTargetPos(theEgg->m_pos);

  if (arrived)
  {
    theEgg->Fertilise(m_spiritId);
    m_spiritId = -1;
    m_eggId.SetInvalid();
  }

  END_PROFILE(g_app->m_profiler, "AdvanceToEgg");
  return false;
}

bool Virii::SearchForEnemies()
{
  START_PROFILE(g_app->m_profiler, "SearchForEnemies");

  auto unit = static_cast<ViriiUnit*>(g_app->m_location->GetUnit(m_id));
  if (unit && !unit->m_enemiesFound)
  {
    END_PROFILE(g_app->m_profiler, "SearchForEnemies");
    return false;
  }

  WorldObjectId bestEnemyId = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, VIRII_MINSEARCHRANGE,
                                                                            VIRII_MAXSEARCHRANGE, m_id.GetTeamId());
  Entity* enemy = g_app->m_location->GetEntity(bestEnemyId);

  if (enemy && !enemy->m_dead)
  {
    m_enemyId = bestEnemyId;
    m_state = StateAttacking;
    END_PROFILE(g_app->m_profiler, "SearchForEnemies");
    return true;
  }

  END_PROFILE(g_app->m_profiler, "SearchForEnemies");
  return false;
}

bool Virii::SearchForSpirits()
{
  START_PROFILE(g_app->m_profiler, "SearchForSpirits");

  Spirit* found = nullptr;
  int spiritId = -1;
  float closest = 999999.9f;

  for (int i = 0; i < g_app->m_location->m_spirits.Size(); ++i)
  {
    if (g_app->m_location->m_spirits.ValidIndex(i))
    {
      Spirit* s = g_app->m_location->m_spirits.GetPointer(i);
      float theDist = (s->m_pos - m_pos).Mag();

      if (theDist <= VIRII_MAXSEARCHRANGE && theDist < closest && s->NumNearbyEggs() > 0 && (s->m_state == Spirit::StateBirth || s->
        m_state == Spirit::StateFloating))
      {
        found = s;
        spiritId = i;
        closest = theDist;
      }
    }
  }

  if (found)
  {
    m_spiritId = spiritId;
    m_state = StateToSpirit;
    END_PROFILE(g_app->m_profiler, "SearchForSpirits");
    return true;
  }

  END_PROFILE(g_app->m_profiler, "SearchForSpirits");
  return false;
}

WorldObjectId Virii::FindNearbyEgg(int _spiritId, float _autoAccept)
{
  if (!g_app->m_location->m_spirits.ValidIndex(_spiritId))
    return WorldObjectId();

  Spirit* spirit = g_app->m_location->m_spirits.GetPointer(_spiritId);
  if (!spirit)
    return WorldObjectId();

  WorldObjectId* m_nearbyEggs = spirit->GetNearbyEggs();
  int numNearbyEggs = spirit->NumNearbyEggs();

  WorldObjectId eggId;
  float closest = 999999.9f;

  for (int i = 0; i < numNearbyEggs; ++i)
  {
    WorldObjectId thisEggId = m_nearbyEggs[i];
    auto egg = static_cast<Egg*>(g_app->m_location->GetEntitySafe(thisEggId, EntityType::TypeEgg));

    if (egg && egg->m_state == Egg::StateDormant)
    {
      float theDist = (egg->m_pos - spirit->m_pos).Mag();
      if (theDist <= _autoAccept)
        return thisEggId;
      if (theDist < closest)
      {
        eggId = thisEggId;
        closest = theDist;
      }
    }
  }

  return eggId;
}

WorldObjectId Virii::FindNearbyEgg(const LegacyVector3& _pos)
{
  int numFound;
  WorldObjectId* ids = g_app->m_location->m_entityGrid->GetFriends(_pos.x, _pos.z, VIRII_MAXSEARCHRANGE, &numFound,
                                                                   m_id.GetTeamId());

  //
  // Build a list of candidates within range

  LList<WorldObjectId> eggIds;

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = ids[i];
    auto egg = static_cast<Egg*>(g_app->m_location->GetEntitySafe(id, EntityType::TypeEgg));

    if (egg && egg->m_state == Egg::StateDormant && egg->m_onGround)
      eggIds.PutData(id);
  }

  //
  // Chose one randomly

  if (eggIds.Size() > 0)
  {
    int chosenIndex = syncfrand(eggIds.Size());
    WorldObjectId* eggId = eggIds.GetPointer(chosenIndex);
    return *eggId;
  }
  return WorldObjectId();
}

bool Virii::SearchForEggs()
{
  START_PROFILE(g_app->m_profiler, "SearchForEggs");

  WorldObjectId eggId = FindNearbyEgg(m_pos);

  if (eggId.IsValid())
  {
    m_eggId = eggId;
    m_state = StateToEgg;
    END_PROFILE(g_app->m_profiler, "SearchForEggs");
    return true;
  }

  END_PROFILE(g_app->m_profiler, "SearchFprEggs");
  return false;
}

bool Virii::SearchForIdleDirection()
{
  START_PROFILE(g_app->m_profiler, "SearchForIdleDir");

  float distToSpawnPoint = (m_pos - m_spawnPoint).Mag();
  float chanceOfReturn = (distToSpawnPoint / m_roamRange);
  if (chanceOfReturn < 0.75f)
    chanceOfReturn = 0.0f;
  if (chanceOfReturn >= 1.0f || syncfrand(1.0f) <= chanceOfReturn)
  {
    // We have strayed too far from our spawn point
    // So head back there now
    LegacyVector3 newDirection = (m_spawnPoint - m_pos);
    newDirection.y = 0;
    if (newDirection.x < -0.5f)
      newDirection.x = -1.0f;
    else if (newDirection.x > 0.5f)
      newDirection.x = 1.0f;
    else
      newDirection.x = 0.0f;
    if (newDirection.z < -0.5f)
      newDirection.z = -1.0f;
    else if (newDirection.z > 0.5f)
      newDirection.z = 1.0f;
    else
      newDirection.z = 0.0f;
    newDirection.SetLength(m_stats[StatSpeed]);
    LegacyVector3 nextPos = m_pos + newDirection * m_retargetTimer;
    nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
    m_wayPoint = nextPos;
    m_state = StateIdle;
    RecordHistoryPosition(true);
    g_app->m_soundSystem->TriggerEntityEvent(this, "ChangeDirection");
    END_PROFILE(g_app->m_profiler, "SearchForIdleDir");
    return true;
  }
  int attempts = 0;
  while (attempts < 3)
  {
    ++attempts;

    int x = (static_cast<int>(syncfrand(3.0f)) - 1);
    int z = (static_cast<int>(syncfrand(3.0f)) - 1);
    LegacyVector3 newVel(x, 0.0f, z);
    newVel.Normalize();
    newVel *= m_stats[StatSpeed];

    LegacyVector3 nextPos = m_pos + newVel * m_retargetTimer;
    nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
    if (nextPos.y > 0.0f)
    {
      m_wayPoint = nextPos;
      m_state = StateIdle;
      RecordHistoryPosition(true);
      g_app->m_soundSystem->TriggerEntityEvent(this, "ChangeDirection");
      END_PROFILE(g_app->m_profiler, "SearchForIdleDir");
      return true;
    }
  }

  END_PROFILE(g_app->m_profiler, "SearchForIdleDir");
  return false;
}

LegacyVector3 Virii::AdvanceDeadPositionVector(int _index, const LegacyVector3& _pos, float _time)
{
  float distance = 10.0f * _time;
  LegacyVector3 result = _pos;

  switch (_index % 8)
  {
    case 0:
      result += LegacyVector3(-distance, 0, 0);
      break;
    case 1:
      result += LegacyVector3(-distance, 0, distance);
      break;
    case 2:
      result += LegacyVector3(0, 0, distance);
      break;
    case 3:
      result += LegacyVector3(distance, 0, distance);
      break;
    case 4:
      result += LegacyVector3(distance, 0, 0);
      break;
    case 5:
      result += LegacyVector3(distance, 0, -distance);
      break;
    case 6:
      result += LegacyVector3(0, 0, -distance);
      break;
    case 7:
      result += LegacyVector3(-distance, 0, -distance);
      break;
  }

  return result;
}

bool Virii::AdvanceDead()
{
  for (int i = 0; i < m_positionHistory.Size(); ++i)
  {
    LegacyVector3* thisPos = &m_positionHistory[i]->m_pos;
    *thisPos = AdvanceDeadPositionVector(i, *thisPos, SERVER_ADVANCE_PERIOD);
  }
  return true;
}

bool Virii::IsInView()
{
  LegacyVector3 centerPos = m_pos;
  float radiusSqd = 0.0f;

  for (int i = 0; i < m_positionHistory.Size(); ++i)
  {
    LegacyVector3 pos = m_positionHistory[i]->m_pos;
    float distance = (pos - centerPos).MagSquared();
    if (distance > radiusSqd)
      radiusSqd = distance;
  }

  float radius = sqrtf(radiusSqd);
  return g_app->m_camera->SphereInViewFrustum(centerPos, radius);
}

void Virii::Render(float predictionTime, int teamId, int _detail)
{
  predictionTime += SERVER_ADVANCE_PERIOD;

  LegacyVector3 predictedPos = m_pos + m_vel * predictionTime;
  if (m_onGround && _detail == 1)
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z) + m_hoverHeight;

  float health = static_cast<float>(m_stats[StatHealth]) / EntityBlueprint::GetStat(m_entityType, StatHealth);

  RGBAColor wormColor = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour * health;
  RGBAColor glowColor(200, 100, 100);
  wormColor.a = 200;
  glowColor.a = 150;

  if (m_dead)
  {
    wormColor.r = wormColor.g = wormColor.b = 250;
    wormColor.a = 100 * m_stats[StatHealth] / 100.0f;

    glowColor.r = glowColor.g = glowColor.b = 250;
    glowColor.a = 250 * m_stats[StatHealth] / 100.0f;
  }

  LegacyVector3 landNormal = g_upVector;
  if (_detail == 1)
    landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);

  ViriiHistory prevPos;
  prevPos.m_pos = predictedPos;
  prevPos.m_right = -m_front ^ landNormal;
  LegacyVector3 firstPos;
  if (m_positionHistory.Size() > 0)
    firstPos = m_positionHistory[0]->m_pos;
  prevPos.m_distance = (predictedPos - firstPos).Mag();
  prevPos.m_glowDiff = (predictedPos - firstPos).SetLength(10.0f);

#define wormWidth       3.0f
#define wormTexW        32.0f/(32.0f+128.0f)
#define wormTexHeight   (10.0f * 2)/1024.0f

#define glowWidth       12.0f
#define glowTexXpos     32.0f/(32.0f+128.0f)
#define glowTexH        128.0f/512.0f

  float wormTexYpos = 0.0f;
  float skippedDistance = 0.0f;

  int lastIndex = m_positionHistory.Size();
  if (_detail > 1)
  {
    int amountToChop = _detail - 1;
    amountToChop = std::min(amountToChop, 2);
    lastIndex *= (1.0f - 0.25f * amountToChop);
  }

  for (int i = 0; i < lastIndex; i++)
  {
    ViriiHistory* history = m_positionHistory[i];

    if (!history->m_required)
    {
      // This worm is a long way away, so we can skip this point in his history
      skippedDistance += history->m_distance;
      continue;
    }

    LegacyVector3& pos = history->m_pos;
    LegacyVector3 wormRightAngle = prevPos.m_right * wormWidth;
    LegacyVector3 glowRightAngle = prevPos.m_right * glowWidth;
    float distance = prevPos.m_distance + skippedDistance;
    const LegacyVector3& glowDiff = prevPos.m_glowDiff;

    skippedDistance = 0.0f;

    //
    // Worm shape

    int newCol = wormColor.a - distance;
    if (newCol < 0)
      newCol = 0;
    wormColor.a = newCol;
    glColor4ubv(wormColor.GetData());

    glTexCoord2f(0.0f, wormTexYpos);
    glVertex3fv((prevPos.m_pos - wormRightAngle).GetData());
    glTexCoord2f(wormTexW, wormTexYpos);
    glVertex3fv((prevPos.m_pos + wormRightAngle).GetData());
    wormTexYpos += (distance * 6) / 512.0f;
    glTexCoord2f(wormTexW, wormTexYpos);
    glVertex3fv((pos + wormRightAngle).GetData());
    glTexCoord2f(0.0f, wormTexYpos);
    glVertex3fv((pos - wormRightAngle).GetData());

    //
    // Glow effect

    if (_detail < 4)
    {
      newCol = glowColor.a - distance;
      if (newCol < 0)
        newCol = 0;
      glowColor.a = newCol;
      glColor4ubv(glowColor.GetData());

      glTexCoord2f(glowTexXpos, 0.0f);
      glVertex3fv((prevPos.m_pos - glowRightAngle + glowDiff).GetData());
      glTexCoord2f(1.0f, 0.0f);
      glVertex3fv((prevPos.m_pos + glowRightAngle + glowDiff).GetData());

      glTexCoord2f(1.0f, glowTexH);
      glVertex3fv((pos + glowRightAngle - glowDiff).GetData());
      glTexCoord2f(glowTexXpos, glowTexH);
      glVertex3fv((pos - glowRightAngle - glowDiff).GetData());
    }

    prevPos = *history;
  }
}
