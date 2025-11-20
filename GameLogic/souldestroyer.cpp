#include "pch.h"
#include "souldestroyer.h"
#include "GameApp.h"
#include "camera.h"
#include "entity_grid.h"
#include "explosion.h"
#include "globals.h"
#include "hi_res_time.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "matrix34.h"
#include "Renderer.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"

Shape* SoulDestroyer::s_shapeHead = nullptr;
Shape* SoulDestroyer::s_shapeTail = nullptr;
ShapeMarker* SoulDestroyer::s_tailMarker = nullptr;

SoulDestroyer::SoulDestroyer()
  : Entity(EntityType::TypeSoulDestroyer),
    m_retargetTimer(0.0f),
    m_panic(0.0f)
{
  if (!s_shapeTail || !s_shapeHead)
  {
    s_shapeTail = Resource::GetShape("souldestroyertail.shp");
    s_shapeHead = Resource::GetShape("souldestroyerhead.shp");

    s_tailMarker = s_shapeHead->m_rootFragment->LookupMarker("MarkerTail");
  }

  m_shape = s_shapeHead;

  float range = 10.0f;
  for (int i = 0; i < SOULDESTROYER_MAXSPIRITS; ++i)
    m_spiritPosition[i] = LegacyVector3(syncsfrand(range), syncsfrand(range), syncsfrand(range));

  m_routeTriggerDistance = 50.0f;
}

void SoulDestroyer::Begin()
{
  Entity::Begin();

  m_up = g_upVector;
  m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

  // This line to make it a bit easier to hit them with missiles
  m_radius *= 1.5f;
}

void SoulDestroyer::ChangeHealth(int _amount)
{
  if (!m_dead && _amount < 0)
  {
    Entity::ChangeHealth(_amount);

    float fractionDead = 1.0f - static_cast<float>(m_stats[StatHealth]) / EntityBlueprint::GetStat(EntityType::TypeSoulDestroyer, StatHealth);
    fractionDead = std::max(fractionDead, 0.0f);
    fractionDead = std::min(fractionDead, 1.0f);
    if (m_dead)
      fractionDead = 1.0f;

    Panic(2.0f + syncfrand(2.0f));

    Matrix34 transform(m_front, m_up, m_pos);
    g_explosionManager.AddExplosion(m_shape, transform, fractionDead);

    if (m_dead)
    {
      // We just died
      for (int i = 1; i < m_positionHistory.Size(); i += 1)
      {
        LegacyVector3 pos1 = *m_positionHistory.GetPointer(i);
        LegacyVector3 pos2 = *m_positionHistory.GetPointer(i - 1);

        LegacyVector3 pos = pos1 + (pos2 - pos1);
        LegacyVector3 front = (pos2 - pos1).Normalize();
        LegacyVector3 right = front ^ g_upVector;
        LegacyVector3 up = right ^ front;

        float scale = 1.0f - (static_cast<float>(i) / static_cast<float>(m_positionHistory.Size()));
        scale *= 1.5f;
        if (i == m_positionHistory.Size() - 1)
          scale = 0.8f;
        scale = std::max(scale, 0.5f);

        Matrix34 tailMat(front, up, pos);
        tailMat.u *= scale;
        tailMat.r *= scale;
        tailMat.f *= scale;

        g_explosionManager.AddExplosion(m_shape, tailMat, 1.0f);
      }
    }
  }
}

bool SoulDestroyer::Advance(Unit* _unit)
{
  if (m_dead)
    return AdvanceDead(_unit);

  if (m_panic > 0.0f)
  {
    m_targetEntity.SetInvalid();
    if (syncfrand(10.0f) < 5.0f)
      SearchForRetreatPosition();
    m_panic -= SERVER_ADVANCE_PERIOD;
  }
  else if (m_targetEntity.IsValid())
  {
    WorldObject* target = g_app->m_location->GetEntity(m_targetEntity);
    if (target)
    {
      float distance = (target->m_pos - m_pos).Mag();
      m_targetPos = target->m_pos;

      if (distance > SOULDESTROYER_MAXSEARCHRANGE)
        m_targetEntity.SetInvalid();
    }
    else
      m_targetEntity.SetInvalid();
  }

  m_retargetTimer -= SERVER_ADVANCE_PERIOD;

  bool arrived = AdvanceToTargetPosition();
  if (arrived || m_targetPos == g_zeroVector || m_retargetTimer < 0.0f)
  {
    m_retargetTimer = 5.0f;
    bool found = false;
    if (!found && m_panic < 0.1f)
      found = SearchForTargetEnemy();
    if (!found)
      found = SearchForRandomPosition();
  }

  RecordHistoryPosition();

  if (m_panic < 0.1f)
    Attack(m_pos);

  return Entity::Advance(_unit);
}

void SoulDestroyer::Attack(const LegacyVector3& _pos)
{
  int numFound;
  WorldObjectId* ids = g_app->m_location->m_entityGrid->GetEnemies(_pos.x, _pos.z, SOULDESTROYER_DAMAGERANGE, &numFound,
                                                                   m_id.GetTeamId());

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = ids[i];
    auto entity = g_app->m_location->GetEntity(id);
    bool killed = false;

    LegacyVector3 pushVector = (entity->m_pos - _pos);
    float distance = pushVector.Mag();
    if (distance < SOULDESTROYER_DAMAGERANGE)
    {
      g_app->m_soundSystem->TriggerEntityEvent(this, "Attack");

      pushVector.SetLength(SOULDESTROYER_DAMAGERANGE - distance);

      g_app->m_location->m_entityGrid->RemoveObject(id, entity->m_pos.x, entity->m_pos.z, entity->m_radius);
      entity->m_pos += pushVector;
      g_app->m_location->m_entityGrid->AddObject(id, entity->m_pos.x, entity->m_pos.z, entity->m_radius);

      bool dead = entity->m_dead;
      entity->ChangeHealth((SOULDESTROYER_DAMAGERANGE - distance) * -50.0f);
      if (!dead && entity->m_dead)
        killed = true;
    }

    if (killed && entity->m_entityType == EntityType::TypeDarwinian)
    {
      // Eat the spirit
      int spiritIndex = g_app->m_location->GetSpirit(id);
      if (spiritIndex != -1)
      {
        g_app->m_location->m_spirits.MarkNotUsed(spiritIndex);
        if (m_spirits.NumUsed() < SOULDESTROYER_MAXSPIRITS)
          m_spirits.PutData(static_cast<float>(GetHighResTime()));
        else
        {
          int index = syncrand() % SOULDESTROYER_MAXSPIRITS;
          m_spirits.PutData(static_cast<float>(GetHighResTime()), index);
        }
      }

      // Create a zombie
      auto zombie = new Zombie();
      zombie->m_pos = entity->m_pos;
      zombie->m_front = entity->m_front;
      zombie->m_up = g_upVector;
      zombie->m_up.RotateAround(zombie->m_front * syncsfrand());
      zombie->m_vel = m_vel * 0.5f;
      zombie->m_vel.y = 20.0f + syncfrand(25.0f);
      int index = g_app->m_location->m_effects.PutData(zombie);
      zombie->m_id.Set(id.GetTeamId(), UNIT_EFFECTS, index, -1);
      zombie->m_id.GenerateUniqueId();
    }
  }
}

void SoulDestroyer::Panic(float _time)
{
  if (m_panic <= 0.0f)
    g_app->m_soundSystem->TriggerEntityEvent(this, "Panic");

  m_panic = std::max(_time, m_panic);
}

bool SoulDestroyer::SearchForRetreatPosition()
{
  WorldObjectId targetId = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, 0.0f, SOULDESTROYER_MAXSEARCHRANGE,
                                                                         m_id.GetTeamId());

  if (targetId.IsValid())
  {
    WorldObject* obj = g_app->m_location->GetEntity(targetId);
    DEBUG_ASSERT(obj);

    float distance = 50.0f;
    LegacyVector3 retreatVector = (m_pos - obj->m_pos).Normalize();
    float angle = syncsfrand(M_PI * 1.0f);
    retreatVector.RotateAroundY(angle);
    m_targetPos = m_pos + retreatVector * distance;
    m_targetPos.y = std::min(m_targetPos.y, 300.0f);
    return true;
  }

  return false;
}

bool SoulDestroyer::SearchForTargetEnemy()
{
  if (m_routeId != -1)
  {
    // dont attack darwinians if on a pre-set route
    return false;
  }
  // If we are too close to the ground, we MUST take off
  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  if (fabs(m_pos.y - landHeight) < 50.0f)
  {
    m_targetEntity.SetInvalid();
    return false;
  }

  // If we are too high up we don't see any enemies
  //    if( m_pos.y > landHeight + 100.0f )
  //    {
  //        m_targetEntity.SetInvalid();
  //        return false;
  //    }

  float maxRange = SOULDESTROYER_MAXSEARCHRANGE;
  float minRange = SOULDESTROYER_MINSEARCHRANGE;

  WorldObjectId targetId = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, minRange, maxRange, m_id.GetTeamId());

  if (targetId.IsValid())
  {
    m_targetEntity = targetId;
    g_app->m_soundSystem->TriggerEntityEvent(this, "EnemySighted");
    return true;
  }
  m_targetEntity.SetInvalid();
  return false;
}

bool SoulDestroyer::SearchForRandomPosition()
{
  LegacyVector3 toSpawnPoint = (m_pos - m_spawnPoint);
  toSpawnPoint.y = 0.0f;
  float distToSpawnPoint = toSpawnPoint.Mag();
  float chanceOfReturn = (distToSpawnPoint / m_roamRange);
  if (chanceOfReturn >= 1.0f || syncfrand(1.0f) <= chanceOfReturn)
  {
    // We have strayed too far from our spawn point
    // So head back there now
    LegacyVector3 targetPos = m_spawnPoint;
    targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);
    targetPos.y += 100.0f + syncsfrand(100.0f);

    LegacyVector3 returnVector = (targetPos - m_pos);
    returnVector.SetLength(160.0f);
    m_targetPos = m_pos + returnVector;
  }
  else
  {
    float distance = 160.0f;
    float angle = syncsfrand(2.0f * M_PI);

    m_targetPos = m_pos + LegacyVector3(sinf(angle) * distance, 0.0f, cosf(angle) * distance);
    m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
    m_targetPos.y += (100.0f + syncsfrand(100.0f));
  }

  return true;
}

void SoulDestroyer::RecordHistoryPosition()
{
  Matrix34 mat(m_front, m_up, m_pos);
  LegacyVector3 tailPos = s_tailMarker->GetWorldMatrix(mat).pos;
  m_positionHistory.PutDataAtStart(tailPos);

  //int maxHistorys = 11;
  int maxHistorys = m_roamRange / 30.0f;
  maxHistorys = std::max(9, maxHistorys);
  maxHistorys = std::min(25, maxHistorys);

  for (int i = maxHistorys; i < m_positionHistory.Size(); ++i)
    m_positionHistory.RemoveData(i);
}

bool SoulDestroyer::GetTrailPosition(LegacyVector3& _pos, LegacyVector3& _vel)
{
  if (m_positionHistory.Size() < 2)
    return false;

  LegacyVector3 pos1 = *m_positionHistory.GetPointer(1);
  LegacyVector3 pos2 = *m_positionHistory.GetPointer(0);

  _pos = pos1 + (pos2 - pos1);
  _vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;

  return true;
}

bool SoulDestroyer::AdvanceToTargetPosition()
{
  float amountToTurn = SERVER_ADVANCE_PERIOD * 2.0f;
  LegacyVector3 targetDir = (m_targetPos - m_pos).Normalize();
  if (!m_targetEntity.IsValid())
  {
    LegacyVector3 right1 = m_front ^ m_up;
    targetDir.RotateAround(right1 * sinf(g_gameTime * 6.0f) * 1.5f);
  }

  // Look ahead to see if we're about to hit the ground
  LegacyVector3 forwardPos = m_pos + targetDir * 50.0f;
  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(forwardPos.x, forwardPos.z);
  if (forwardPos.y <= landHeight)
    targetDir = g_upVector;

  LegacyVector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
  actualDir.Normalize();
  float speed = m_stats[StatSpeed];
  speed = 130.0f;

  LegacyVector3 oldPos = m_pos;
  LegacyVector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
  landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(newPos.x, newPos.z);
  //newPos.y = max( newPos.y, landHeight );

  LegacyVector3 moved = newPos - oldPos;
  if (moved.Mag() > speed * SERVER_ADVANCE_PERIOD)
    moved.SetLength(speed * SERVER_ADVANCE_PERIOD);
  newPos = m_pos + moved;

  m_pos = newPos;
  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
  m_front = actualDir;

  LegacyVector3 right = m_front ^ g_upVector;
  m_up = right ^ m_front;

  return (m_pos - m_targetPos).Mag() < 40.0f;
}

void SoulDestroyer::RenderShapes(float _predictionTime)
{
  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
  LegacyVector3 predictedFront = m_front;
  LegacyVector3 predictedUp = m_up;
  LegacyVector3 predictedRight = predictedUp ^ predictedFront;
  predictedFront = predictedRight ^ predictedUp;
  predictedFront.Normalize();
  predictedUp.Normalize();
  Matrix34 mat(predictedFront, predictedUp, predictedPos);

  glEnable(GL_NORMALIZE);
  glDisable(GL_TEXTURE_2D);

  g_app->m_renderer->SetObjectLighting();

  s_shapeHead->Render(_predictionTime, mat);

  for (int i = 1; i < m_positionHistory.Size(); i += 1)
  {
    LegacyVector3 pos1 = *m_positionHistory.GetPointer(i);
    LegacyVector3 pos2 = *m_positionHistory.GetPointer(i - 1);

    LegacyVector3 pos = pos1 + (pos2 - pos1);
    LegacyVector3 front = (pos2 - pos1).Normalize();
    LegacyVector3 right = front ^ g_upVector;
    LegacyVector3 up = right ^ front;
    LegacyVector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
    pos += vel * _predictionTime;

    float scale = 1.0f - (static_cast<float>(i) / static_cast<float>(m_positionHistory.Size()));
    scale *= 1.5f;
    if (i == m_positionHistory.Size() - 1)
      scale = 0.8f;
    scale = std::max(scale, 0.5f);

    Matrix34 tailMat(front, up, pos);
    tailMat.u *= scale;
    tailMat.r *= scale;
    tailMat.f *= scale;

    s_shapeTail->Render(_predictionTime, tailMat);
  }

  g_app->m_renderer->UnsetObjectLighting();

  glDisable(GL_NORMALIZE);
}

void SoulDestroyer::Render(float _predictionTime)
{
  _predictionTime -= SERVER_ADVANCE_PERIOD;

  if (!m_dead)
  {
    glDisable(GL_TEXTURE_2D);

    LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
    LegacyVector3 predictedFront = m_front;
    LegacyVector3 predictedUp = m_up;
    LegacyVector3 predictedRight = predictedUp ^ predictedFront;
    predictedFront = predictedRight ^ predictedUp;

    RenderShapes(_predictionTime);

    //
    // Render shadows

    BeginRenderShadow();
    RenderShadow(predictedPos, 50.0f);

    for (int i = 1; i < m_positionHistory.Size(); i += 1)
    {
      LegacyVector3 pos1 = *m_positionHistory.GetPointer(i);
      LegacyVector3 pos2 = *m_positionHistory.GetPointer(i - 1);

      LegacyVector3 pos = pos1 + (pos2 - pos1);
      LegacyVector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
      pos += vel * _predictionTime;

      float scale = 1.0f - (static_cast<float>(i) / static_cast<float>(m_positionHistory.Size()));
      scale *= 1.5f;
      if (i == m_positionHistory.Size() - 1)
        scale = 0.8f;
      scale = std::max(scale, 0.5f);

      RenderShadow(pos, scale * 20.0f);
    }

    EndRenderShadow();

    //
    // Render our spirits

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
    glDepthMask(false);

    float timeNow = GetHighResTime();
    for (int i = 0; i < m_spirits.Size(); ++i)
    {
      if (m_spirits.ValidIndex(i))
      {
        float birthTime = m_spirits[i];
        if (timeNow >= birthTime + 60.0f)
          m_spirits.MarkNotUsed(i);
        else
        {
          float alpha = 1.0f - (timeNow - m_spirits[i]) / 60.0f;
          alpha = std::min(alpha, 1.0f);
          alpha = std::max(alpha, 0.0f);
          LegacyVector3 pos = m_pos + m_spiritPosition[i];
          pos += m_vel * _predictionTime;
          RenderSpirit(pos, alpha);
        }
      }
    }

    glDepthMask(true);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    //glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    //g_editorFont.DrawText3DCenter( predictedPos+LegacyVector3(0,50,0), 20, "%2.2f", m_panic );
  }
}

void SoulDestroyer::RenderSpirit(const LegacyVector3& _pos, float _alpha)
{
  LegacyVector3 pos = _pos;

  int innerAlpha = 200 * _alpha;
  int outerAlpha = 100 * _alpha;
  int spiritInnerSize = 4 * _alpha;
  int spiritOuterSize = 12 * _alpha;

  RGBAColor colour(100, 50, 50);
  float distToParticle = (g_app->m_camera->GetPos() - pos).Mag();

  float size = spiritInnerSize / sqrtf(sqrtf(distToParticle));
  glColor4ub(colour.r, colour.g, colour.b, innerAlpha);

  glBegin(GL_QUADS);
  glVertex3fv((pos - g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetRight() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos - g_app->m_camera->GetRight() * size).GetData());
  glEnd();

  size = spiritOuterSize / sqrtf(sqrtf(distToParticle));
  glColor4ub(colour.r, colour.g, colour.b, outerAlpha);
  glBegin(GL_QUADS);
  glVertex3fv((pos - g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetRight() * size).GetData());
  glVertex3fv((pos + g_app->m_camera->GetUp() * size).GetData());
  glVertex3fv((pos - g_app->m_camera->GetRight() * size).GetData());
  glEnd();
}

void SoulDestroyer::SetWaypoint(const LegacyVector3 _waypoint) { m_targetPos = _waypoint; }

Zombie::Zombie()
  : Effect(EffectType::EffectZombie),
    m_life(0.0f)
{
  m_positionOffset = syncfrand(10.0f);
  m_xaxisRate = syncfrand(2.0f);
  m_yaxisRate = syncfrand(2.0f);
  m_zaxisRate = syncfrand(2.0f);
}

bool Zombie::Advance()
{
  m_life += SERVER_ADVANCE_PERIOD;

  if (m_life > 600.0f)
    return true;

  m_vel *= 0.9f;

  //
  // Make me hover around a bit

  m_positionOffset += SERVER_ADVANCE_PERIOD;
  m_xaxisRate += syncsfrand(1.0f);
  m_yaxisRate += syncsfrand(1.0f);
  m_zaxisRate += syncsfrand(1.0f);
  if (m_xaxisRate > 2.0f)
    m_xaxisRate = 2.0f;
  if (m_xaxisRate < 0.0f)
    m_xaxisRate = 0.0f;
  if (m_yaxisRate > 2.0f)
    m_yaxisRate = 2.0f;
  if (m_yaxisRate < 0.0f)
    m_yaxisRate = 0.0f;
  if (m_zaxisRate > 2.0f)
    m_zaxisRate = 2.0f;
  if (m_zaxisRate < 0.0f)
    m_zaxisRate = 0.0f;

  m_hover.x = sinf(m_positionOffset) * m_xaxisRate;
  m_hover.y = sinf(m_positionOffset) * m_yaxisRate;
  m_hover.z = sinf(m_positionOffset) * m_zaxisRate;

  m_pos += m_hover * SERVER_ADVANCE_PERIOD;
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  return false;
}

void Zombie::Render(float _predictionTime)
{
  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
  predictedPos += m_hover * _predictionTime;

  LegacyVector3 predictedFront = m_front;
  LegacyVector3 predictedUp = m_up;
  LegacyVector3 predictedRight = predictedFront ^ predictedUp;

  float size = 5.0f;

  float alpha = 1.0f - (m_life / 10.0f);
  alpha = std::max(0.1f, alpha);
  alpha = std::min(0.7f, alpha);

  float outerAlpha = (0.7f - alpha) * 0.1f;

  float timeRemaining = 600.0f - m_life;
  if (timeRemaining < 100.0f)
  {
    alpha *= timeRemaining * 0.01f;
    outerAlpha *= timeRemaining * 0.01f;
  }

  glDisable(GL_CULL_FACE);
  glColor4f(0.9f, 0.9f, 1.0f, alpha);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites\\ghost.bmp"));

  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex3fv((predictedPos - size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 0);
  glVertex3fv((predictedPos + size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 1);
  glVertex3fv((predictedPos + size * predictedRight + size * predictedUp).GetData());
  glTexCoord2i(0, 1);
  glVertex3fv((predictedPos - size * predictedRight + size * predictedUp).GetData());
  glEnd();

  size *= 2.0f;
  glColor4f(0.9f, 0.9f, 1.0f, outerAlpha);
  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex3fv((predictedPos - size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 0);
  glVertex3fv((predictedPos + size * predictedRight - size * predictedUp).GetData());
  glTexCoord2i(1, 1);
  glVertex3fv((predictedPos + size * predictedRight + size * predictedUp).GetData());
  glTexCoord2i(0, 1);
  glVertex3fv((predictedPos - size * predictedRight + size * predictedUp).GetData());
  glEnd();

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}
