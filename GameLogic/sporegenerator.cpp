#include "pch.h"
#include "sporegenerator.h"
#include "GameApp.h"
#include "camera.h"
#include "explosion.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "matrix34.h"
#include "Renderer.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"

#define SPOREGENERATOR_HOVERHEIGHT          100.0f
#define SPOREGENERATOR_EGGLAYHEIGHT         20.0f
#define SPOREGENERATOR_SPIRITSEARCHRANGE    200.0f

SporeGenerator::SporeGenerator()
  : Entity(EntityType::TypeSporeGenerator),
    m_retargetTimer(0.0f),
    m_eggTimer(0.0f),
    m_state(StateIdle)
{
  m_shape = Resource::GetShape("sporegenerator.shp");
  m_eggMarker = m_shape->m_rootFragment->LookupMarker("MarkerEggs");

  for (int i = 0; i < SPOREGENERATOR_NUMTAILS; ++i)
  {
    char name[256];
    sprintf(name, "MarkerTail0%d", i + 1);
    m_tail[i] = m_shape->m_rootFragment->LookupMarker(name);
  }
}

void SporeGenerator::Begin()
{
  Entity::Begin();

  m_targetPos = m_pos;
  m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  m_pos.y += SPOREGENERATOR_HOVERHEIGHT;
}

void SporeGenerator::ChangeHealth(int _amount)
{
  if (!m_dead && _amount < -1)
  {
    Entity::ChangeHealth(_amount);

    float fractionDead = 1.0f - static_cast<float>(m_stats[StatHealth]) / EntityBlueprint::GetStat(EntityType::TypeSporeGenerator, StatHealth);
    fractionDead = std::max(fractionDead, 0.0f);
    fractionDead = std::min(fractionDead, 1.0f);
    if (m_dead)
      fractionDead = 1.0f;

    Matrix34 transform(m_front, g_upVector, m_pos);
    g_explosionManager.AddExplosion(m_shape, transform, fractionDead);

    m_state = StatePanic;
    m_retargetTimer = 10.0f;
    m_vel = g_upVector * m_stats[StatSpeed] * 3.0f;
  }
}

bool SporeGenerator::SearchForRandomPos()
{
  float distToSpawnPoint = (m_pos - m_spawnPoint).Mag();
  float chanceOfReturn = (distToSpawnPoint / m_roamRange);
  if (chanceOfReturn >= 1.0f || syncfrand(1.0f) <= chanceOfReturn)
  {
    // We have strayed too far from our spawn point
    // So head back there now
    LegacyVector3 returnVector = m_spawnPoint - m_pos;
    returnVector.SetLength(200.0f);
    m_targetPos = m_pos + returnVector;
  }
  else
    m_targetPos = m_pos + LegacyVector3(syncsfrand(200.0f), 0.0f, syncsfrand(200.0f));

  m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
  m_targetPos.y += SPOREGENERATOR_HOVERHEIGHT * (1.0f + syncsfrand(0.3f));

  m_state = StateIdle;

  //
  // Give us a random chance we will lay some eggs anyway

  if (syncfrand(100.0f) < 10.0f + 20.0f * g_app->m_difficultyLevel / 10.0f)
  {
    m_state = StateEggLaying;
    m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
    m_targetPos.y += SPOREGENERATOR_EGGLAYHEIGHT;
  }

  return true;
}

bool SporeGenerator::SearchForSpirits()
{
  Spirit* found = nullptr;
  int foundIndex = -1;
  float nearest = 9999.9f;

  for (int i = 0; i < g_app->m_location->m_spirits.Size(); ++i)
  {
    if (g_app->m_location->m_spirits.ValidIndex(i))
    {
      Spirit* s = g_app->m_location->m_spirits.GetPointer(i);
      if (s->NumNearbyEggs() < 3 && s->m_pos.y > 0.0f)
      {
        float theDist = (s->m_pos - m_pos).Mag();

        if (theDist <= SPOREGENERATOR_SPIRITSEARCHRANGE && theDist < nearest && s->m_state == Spirit::StateFloating)
        {
          found = s;
          foundIndex = i;
          nearest = theDist;
        }
      }
    }
  }

  if (found)
  {
    m_spiritId = foundIndex;
    m_targetPos = found->m_pos;
    m_targetPos += LegacyVector3(syncsfrand(60.0f), 0.0f, syncsfrand(60.0f));
    m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
    m_targetPos.y += SPOREGENERATOR_EGGLAYHEIGHT;
    m_state = StateEggLaying;
  }

  return found;
}

bool SporeGenerator::AdvancePanic()
{
  m_retargetTimer -= SERVER_ADVANCE_PERIOD;

  if (m_retargetTimer <= 0.0f)
  {
    m_state = StateIdle;
    return false;
  }

  m_targetPos = m_pos + LegacyVector3(syncsfrand(100.0f), syncfrand(10.0f), syncsfrand(100.0f));

  AdvanceToTargetPosition();

  return false;
}

bool SporeGenerator::AdvanceToTargetPosition()
{
  float heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  LegacyVector3 toTarget = m_pos - m_targetPos;
  float distanceToTarget = toTarget.Mag();

  LegacyVector3 targetVel;

  float speed = m_stats[StatSpeed];
  if (distanceToTarget < 50.0f)
    speed *= distanceToTarget / 50.0f;
  targetVel = (m_targetPos - m_pos).SetLength(speed);

  float factor1 = SERVER_ADVANCE_PERIOD * 0.5f;
  float factor2 = 1.0f - factor1;

  m_vel = m_vel * factor2 + targetVel * factor1;

  LegacyVector3 right = m_front ^ g_upVector;
  m_vel.RotateAround(right * sinf(g_gameTime * 3.0f) * SERVER_ADVANCE_PERIOD * 1.5f);

  m_front = m_vel;
  m_front.y = 0.0f;
  m_front.Normalize();
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  return (distanceToTarget < 10.0f);
}

bool SporeGenerator::Advance(Unit* _unit)
{
  bool amIDead = Entity::Advance(_unit);

  if (!amIDead && !m_dead)
  {
    switch (m_state)
    {
      case StateIdle:
        amIDead = AdvanceIdle();
        break;
      case StateEggLaying:
        amIDead = AdvanceEggLaying();
        break;
      case StatePanic:
        amIDead = AdvancePanic();
        break;
    }
  }

  return amIDead;
}

bool SporeGenerator::AdvanceIdle()
{
  m_retargetTimer -= SERVER_ADVANCE_PERIOD;
  if (m_retargetTimer <= 0.0f)
  {
    m_retargetTimer = 6.0f;

    bool targetFound = false;
    if (!targetFound)
      targetFound = SearchForSpirits();
    if (!targetFound)
      targetFound = SearchForRandomPos();

    if (m_state != StateIdle)
      return false;
  }

  bool arrived = AdvanceToTargetPosition();

  return false;
}

bool SporeGenerator::AdvanceEggLaying()
{
  //
  // Advance to where we think the spirits are

  bool arrived = AdvanceToTargetPosition();

  //
  // Lay eggs if we are in range

  float distance = (m_targetPos - m_pos).Mag();
  if (distance < 50.0f)
  {
    m_eggTimer -= SERVER_ADVANCE_PERIOD;
    if (m_eggTimer <= 0.0f)
    {
      m_eggTimer = 2.0f + syncfrand(2.0f) - 1.0 * static_cast<float>(g_app->m_difficultyLevel) / 10.0f;
      Matrix34 mat(m_front, g_upVector, m_pos);
      Matrix34 eggLayMat = m_eggMarker->GetWorldMatrix(mat);
      g_app->m_location->SpawnEntities(eggLayMat.pos, m_id.GetTeamId(), -1, EntityType::TypeEgg, 1, m_vel, 0.0f);
      g_app->m_soundSystem->TriggerEntityEvent(this, "LayEgg");
    }
  }

  //
  // See if enough eggs are present

  if (arrived)
  {
    bool targetFound = false;
    if (!targetFound)
      targetFound = SearchForSpirits();
    if (!targetFound)
      targetFound = SearchForRandomPos();
  }

  return false;
}

void SporeGenerator::RenderTail(const LegacyVector3& _from, const LegacyVector3& _to, float _size)
{
  //RenderArrow( _from, _to, _size, RGBAColor(255,50,50,255) );

  LegacyVector3 camToOurPos = g_app->m_camera->GetPos() - _from;
  LegacyVector3 lineOurPos = camToOurPos ^ (_from - _to);
  lineOurPos.SetLength(_size);

  LegacyVector3 lineVector = (_to - _from).Normalize();
  LegacyVector3 right = lineVector ^ g_upVector;
  LegacyVector3 normal = right ^ lineVector;
  normal.Normalize();

  glNormal3fv(normal.GetData());

  glVertex3fv((_from - lineOurPos).GetData());
  glVertex3fv((_from + lineOurPos).GetData());
  glVertex3fv((_to - lineOurPos).GetData());
  glVertex3fv((_to + lineOurPos).GetData());
}

void SporeGenerator::Render(float _predictionTime)
{
  if (m_dead)
    return;

  //    RenderArrow( m_pos, m_pos+m_front*20.0f, 1.0f );
  //    RenderArrow( m_pos, m_targetPos, 2.0f );
  //    g_editorFont.DrawText3DCenter( m_pos+LegacyVector3(0,50,0), 10.0f, "%d", (int) m_stats[StatHealth] );

  auto entityFront = LegacyVector3(0, 0, 1);
  LegacyVector3 entityUp = g_upVector;
  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;

  //
  // 3d Shape

  g_app->m_renderer->SetObjectLighting();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  Matrix34 mat(entityFront, entityUp, predictedPos);

  if (m_renderDamaged)
  {
    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    float thefrand = frand();
    if (thefrand > 0.7f)
      mat.f *= (1.0f - sinf(timeIndex) * 0.5f);
    else if (thefrand > 0.4f)
      mat.u *= (1.0f - sinf(timeIndex) * 0.5f);
    else
      mat.r *= (1.0f - sinf(timeIndex) * 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
  }

  m_shape->Render(_predictionTime, mat);

  //
  // Tails

  glColor4f(0.2f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_COLOR_MATERIAL);

  int numTailParts = 3;
  static LegacyVector3 s_vel;
  s_vel = s_vel * 0.95f + m_vel * 0.05f;

  for (int i = 0; i < SPOREGENERATOR_NUMTAILS; ++i)
  {
    LegacyVector3 prevTailPos = m_tail[i]->GetWorldMatrix(mat).pos;
    LegacyVector3 prevTailDir = (prevTailPos - predictedPos);
    prevTailDir.HorizontalAndNormalize();

    glBegin(GL_QUAD_STRIP);

    for (int j = 0; j < numTailParts; ++j)
    {
      LegacyVector3 thisTailPos = prevTailPos + prevTailDir * 15.0f;
      prevTailDir = (thisTailPos - prevTailPos);
      prevTailDir.HorizontalAndNormalize();

      float timeIndex = g_gameTime + i + j + m_id.GetUniqueId() * 10;
      thisTailPos += LegacyVector3(sinf(timeIndex) * 10.0f, sinf(timeIndex) * 10.0f, sinf(timeIndex) * 10.0f);

      LegacyVector3 vel = s_vel;
      vel.SetLength(10.0f * static_cast<float>(j) / static_cast<float>(numTailParts));
      thisTailPos += vel;
      thisTailPos.y -= 10.0f * static_cast<float>(j) / static_cast<float>(numTailParts);

      if (m_state == StatePanic)
      {
        float panicFraction = 5.0f;
        thisTailPos += LegacyVector3(cosf(g_gameTime * panicFraction + i + j) * 5.0f,
                                     cosf(g_gameTime * panicFraction + i + j) * 5.0f,
                                     cosf(g_gameTime * panicFraction + i + j) * 5.0f);
      }

      float size = 1.0f - static_cast<float>(j) / static_cast<float>(numTailParts);
      size *= 2.0f;
      size += sinf(g_gameTime + i + j) * 0.3f;

      RenderTail(prevTailPos, thisTailPos, size);
      prevTailPos = thisTailPos;
    }

    glEnd();
  }

  glDisable(GL_COLOR_MATERIAL);
  glEnable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  g_app->m_renderer->UnsetObjectLighting();

  //
  // Shadow

  BeginRenderShadow();
  RenderShadow(predictedPos, 20.0f);
  EndRenderShadow();
}

bool SporeGenerator::IsInView() { return g_app->m_camera->SphereInViewFrustum(m_pos + m_centerPos, m_radius); }
