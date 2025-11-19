#include "pch.h"
#include "armour.h"
#include "GameApp.h"
#include "camera.h"
#include "entity_grid.h"
#include "explosion.h"
#include "global_world.h"
#include "gunturret.h"
#include "hi_res_time.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "matrix34.h"
#include "obstruction_grid.h"
#include "ParticleSystem.h"
#include "Renderer.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"
#include "team.h"

Armour::Armour()
  : Entity(EntityType::TypeArmour),
    m_speed(0.0f),
    m_numPassengers(0),
    m_previousUnloadTimer(0.0f),
    m_newOrdersTimer(0.0f),
    m_state(StateIdle)
{
  m_shape = Resource::GetShape("armour.shp");
  m_markerEntrance = m_shape->m_rootFragment->LookupMarker("MarkerEntrance");
  m_markerFlag = m_shape->m_rootFragment->LookupMarker("MarkerFlag");

  m_centerPos = m_shape->CalculateCenter(g_identityMatrix34);
  m_radius = m_shape->CalculateRadius(g_identityMatrix34, m_centerPos);
}

Armour::~Armour()
{
  if (m_id.GetTeamId() != 255)
  {
    Team* team = &g_app->m_location->m_teams[m_id.GetTeamId()];
    team->UnRegisterSpecial(m_id);
  }
}

void Armour::Begin()
{
  Entity::Begin();

  m_wayPoint = m_pos;

  if (m_id.GetTeamId() != 255)
  {
    Team* team = &g_app->m_location->m_teams[m_id.GetTeamId()];
    team->RegisterSpecial(m_id);
  }

  m_flag.SetPosition(m_pos);
  m_flag.SetOrientation(m_front, m_up);
  m_flag.SetSize(20.0f);
  m_flag.Initialize();
}

void Armour::ChangeHealth(int _amount)
{
  bool dead = m_dead;

  if (_amount < 0 && _amount > -1000)
    _amount = -1;

  if (!m_dead)
  {
    if (_amount < 0)
      g_app->m_soundSystem->TriggerEntityEvent(this, "LoseHealth");

    int oldHealth = m_stats[StatHealth];
    int newHealth = oldHealth + _amount;
    newHealth = std::max(newHealth, 0);
    newHealth = std::min(newHealth, 255);
    m_stats[StatHealth] = newHealth;

    int healthBandBefore = static_cast<int>(oldHealth / 20.0f);
    int healthBandAfter = static_cast<int>(newHealth / 20.0f);

    if (healthBandBefore != healthBandAfter || newHealth == 0)
    {
      float fractionDead = 1.0f - static_cast<float>(newHealth) / EntityBlueprint::GetStat(EntityType::TypeArmour, StatHealth);
      Matrix34 bodyMat(m_front, m_up, m_pos);
      g_explosionManager.AddExplosion(m_shape, bodyMat, fractionDead);
    }

    if (newHealth == 0)
    {
      m_dead = true;
      g_app->m_soundSystem->TriggerEntityEvent(this, "Die");
    }
  }
}

void Armour::ConvertToGunTurret()
{
  GunTurret turretTemplate;
  turretTemplate.m_pos = m_pos;
  turretTemplate.m_front = m_front;
  turretTemplate.m_dynamic = true;

  auto turret = static_cast<GunTurret*>(Building::CreateBuilding(BuildingType::TypeGunTurret));
  g_app->m_location->m_buildings.PutData(turret);
  turret->Initialize(&turretTemplate);
  int id = g_app->m_globalWorld->GenerateBuildingId();
  turret->m_id.SetUnitId(UNIT_BUILDINGS);
  turret->m_id.SetUniqueId(id);
  g_app->m_location->m_obstructionGrid->CalculateAll();

  //
  // Explode some polys, to cover the ropey change
  Matrix34 bodyMat(m_front, m_up, m_pos);
  g_explosionManager.AddExplosion(m_shape, bodyMat);
  turret->ExplodeBody();
}

void Armour::AdvanceToTargetPos()
{
  //
  // Turn to face our waypoint

  LegacyVector3 toTarget = m_wayPoint - m_pos;
  toTarget.HorizontalAndNormalize();
  float angle = acosf(toTarget * m_front);

  if ((m_wayPoint - m_pos).Mag() > 10.0f)
  {
    LegacyVector3 rotation = m_front ^ toTarget;
    float rotateSpeed = angle * 0.05f;
    rotation.SetLength(rotateSpeed);
    m_front.RotateAround(rotation);
    m_front.Normalize();
  }

  //
  // Move towards waypoint

  toTarget = m_wayPoint - m_pos;
  toTarget.y = 0.0f;
  float distance = toTarget.Mag();
  if (distance > 100.0f && angle < 1.0f)
  {
    m_speed += 10.0f * SERVER_ADVANCE_PERIOD;
    m_speed = std::min<float>(m_speed, m_stats[StatSpeed]);
  }
  else if (distance > 10.0f)
  {
    float targetSpeed = distance * 0.2f;
    m_speed = m_speed * 0.95f + targetSpeed * 0.05f;
    m_speed = std::min<float>(m_speed, m_stats[StatSpeed]);
    m_speed = std::max(m_speed, 0.0f);
  }
  else
  {
    m_speed -= 5.0f * SERVER_ADVANCE_PERIOD;
    m_speed = std::max(m_speed, 0.0f);
  }

  //
  // Hover above the ground

  LegacyVector3 oldPos = m_pos;
  m_pos += m_front * m_speed * SERVER_ADVANCE_PERIOD;
  PushFromObstructions(m_pos);
  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  if (m_pos.y <= landHeight)
  {
    float factor = SERVER_ADVANCE_PERIOD * 5.0f;
    m_pos.y = m_pos.y * (1.0f - factor) + landHeight * factor;

    factor = SERVER_ADVANCE_PERIOD * 5.0f;
    LegacyVector3 landUp = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
    m_up = m_up * (1.0f - factor) + landUp * factor;

    float distTravelled = (m_pos - oldPos).Mag();
    if (distTravelled > m_speed * SERVER_ADVANCE_PERIOD)
      m_pos = oldPos + (m_pos - oldPos).SetLength(m_speed * SERVER_ADVANCE_PERIOD);
  }
  else
  {
    float heightAboveGround = m_pos.y - landHeight;
    m_vel.y -= 5.0f;
    m_pos.y += m_vel.y * SERVER_ADVANCE_PERIOD;
    m_pos.y = std::max(m_pos.y, landHeight);

    float factor = SERVER_ADVANCE_PERIOD * 0.5f;
    LegacyVector3 landUp = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
    m_up = m_up * (1.0f - factor) + landUp * factor;
  }

  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
}

void Armour::DetectCollisions()
{
  LegacyVector3 pos(m_pos);
  pos += m_vel;
  int numFound;
  WorldObjectId* neighbours = g_app->m_location->m_entityGrid->GetNeighbours(pos.x, pos.z, 30.0f, &numFound);

  LegacyVector3 escapeVector;
  bool collisionDetected = false;

  for (int i = 0; i < numFound; ++i)
  {
    Entity* ent = g_app->m_location->GetEntity(neighbours[i]);
    if (ent->m_entityType == EntityType::TypeArmour && ent->m_id != m_id)
    {
      Entity* entity = g_app->m_location->GetEntity(neighbours[darwiniaRandom() % numFound]);
      DEBUG_ASSERT(entity);
      LegacyVector3 toNeighbour = m_pos - entity->m_pos;
      toNeighbour.y = 0.0f;
      toNeighbour.Normalize();
      escapeVector += toNeighbour;
      collisionDetected = true;
    }
  }

  if (collisionDetected)
    m_wayPoint = m_pos + escapeVector * 40.0f;
}

bool Armour::Advance(Unit* _unit)
{
  if (m_dead)
    return true;

  m_newOrdersTimer += SERVER_ADVANCE_PERIOD;

  AdvanceToTargetPos();
  DetectCollisions();

  //
  // Create some smoke

  float velocity = m_vel.Mag();
  int numSmoke = 1 + static_cast<int>(velocity / 20.0f);
  for (int i = 0; i < numSmoke; ++i)
  {
    LegacyVector3 right = m_up ^ m_front;
    LegacyVector3 vel = m_front;
    vel.RotateAround(m_up * syncfrand(2.0f * M_PI));
    vel.SetLength(syncfrand(5.0f));
    LegacyVector3 pos = m_pos + vel * 2;
    pos.y += 3.0f;
    float size = 50.0f + (syncrand() % 50);
    g_app->m_particleSystem->CreateParticle(pos, vel, Particle::TypeMissileTrail, size);
  }

  //
  // Are we supposed to be converting here?

  if (m_conversionPoint != g_zeroVector)
  {
    float distance = (m_conversionPoint - m_pos).Mag();
    if (distance < 50.0f && m_numPassengers > 0)
    {
      // We are close and need to unload our passengers
      m_state = StateUnloading;
      m_newOrdersTimer = 2.0f;
    }

    if (distance < 10.0f && m_numPassengers == 0)
    {
      ConvertToGunTurret();
      return true;
    }
  }

  return Entity::Advance(_unit);
}

void Armour::SetOrders(const LegacyVector3& _orders)
{
  m_newOrdersTimer = 0.0f;

  float distance = (_orders - m_pos).Mag();

  if (distance > 50.0f)
    SetConversionPoint(_orders);
  else
  {
    static float lastOrdersSet = 0.0;
    float timeNow = GetHighResTime();

    if (timeNow > lastOrdersSet + 0.3f)
    {
      ToggleLoading();
      lastOrdersSet = timeNow;
    }
  }
}

void Armour::SetDirectOrders()
{
  static float lastOrdersSet = 0.0;
  float timeNow = GetHighResTime();

  if (timeNow > lastOrdersSet + 0.3f)
  {
    ToggleLoading();
    lastOrdersSet = timeNow;
  }
}

void Armour::SetWayPoint(const LegacyVector3& _wayPoint)
{
  m_conversionPoint.Zero();

  m_wayPoint = _wayPoint;
  m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);
}

void Armour::SetConversionPoint(const LegacyVector3& _conversionPoint)
{
  m_conversionPoint.Zero();

  LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(_conversionPoint.x, _conversionPoint.z);
  if (landNormal.y > 0.95f)
  {
    SetWayPoint(_conversionPoint);

    m_conversionPoint = _conversionPoint;
    m_conversionPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_conversionPoint.x, m_conversionPoint.z);
  }
}

bool Armour::IsLoading() { return (m_state == StateLoading && m_numPassengers < Capacity() && m_newOrdersTimer > 1.0f); }

bool Armour::IsUnloading()
{
  return (m_state == StateUnloading && m_numPassengers > 0 && m_newOrdersTimer > 1.0f && GetHighResTime() >= m_previousUnloadTimer +
    ARMOUR_UNLOADPERIOD);
}

int Armour::Capacity()
{
  int research = g_app->m_globalWorld->m_research->CurrentLevel(GlobalResearch::TypeArmour);
  switch (research)
  {
    case 0:
    case 1:
      return 10;
    case 2:
      return 20;
    case 3:
      return 30;
    case 4:
      return 40;
  }

  return 10;
}

void Armour::ToggleLoading()
{
  switch (m_state)
  {
    case StateIdle:
      if (m_numPassengers > 0)
        m_state = StateUnloading;
      else
        m_state = StateLoading;
      break;

    case StateUnloading:
      m_state = StateLoading;
      break;

    case StateLoading:
      m_state = StateIdle;
      break;
  }
}

void Armour::AddPassenger()
{
  ++m_numPassengers;

  g_app->m_soundSystem->TriggerEntityEvent(this, "LoadDarwinian");
}

void Armour::RemovePassenger()
{
  --m_numPassengers;
  m_previousUnloadTimer = GetHighResTime();

  g_app->m_soundSystem->TriggerEntityEvent(this, "UnloadDarwinian");
}

void Armour::GetEntrance(LegacyVector3& _exitPos, LegacyVector3& _exitDir)
{
  Matrix34 mat(m_front, m_up, m_pos);
  Matrix34 entranceMat = m_markerEntrance->GetWorldMatrix(mat);
  _exitPos = entranceMat.pos;
  _exitDir = entranceMat.f;
}

void Armour::Render(float _predictionTime)
{
  if (m_dead)
    return;

  //
  // Work out our predicted position

  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
  predictedPos.y += sinf(g_gameTime + m_id.GetUniqueId()) * 2;
  LegacyVector3 predictedUp = m_up;
  predictedUp.x += sinf((g_gameTime + m_id.GetUniqueId()) * 2) * 0.05f;
  predictedUp.z += cosf(g_gameTime + m_id.GetUniqueId()) * 0.05f;

  LegacyVector3 predictedFront = m_front;
  LegacyVector3 right = predictedUp ^ predictedFront;
  predictedUp.Normalize();
  predictedFront = right ^ predictedUp;
  predictedFront.Normalize();

  //
  // Render the tank body

  Matrix34 bodyMat(predictedFront, predictedUp, predictedPos);

  if (m_renderDamaged)
  {
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    if (frand() > 0.5f)
      bodyMat.r *= (1.0f + sinf(g_gameTime) * 0.5f);
    else
      bodyMat.f *= (1.0f + sinf(g_gameTime) * 0.5f);
  }

  g_app->m_renderer->SetObjectLighting();
  m_shape->Render(_predictionTime, bodyMat);
  g_app->m_renderer->UnsetObjectLighting();

  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //
  // Render the flag

  float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
  Matrix34 flagMat = m_markerFlag->GetWorldMatrix(bodyMat);
  m_flag.SetPosition(flagMat.pos);
  m_flag.SetOrientation(predictedFront * -1, predictedUp);
  m_flag.SetSize(20.0f);

  switch (m_state)
  {
    case StateIdle:
      m_flag.SetTexture(Resource::GetTexture("icons\\banner_none.bmp"));
      break;
    case StateUnloading:
      m_flag.SetTexture(Resource::GetTexture("icons\\banner_unload.bmp"));
      break;
    case StateLoading:
      m_flag.SetTexture(Resource::GetTexture("icons\\banner_follow.bmp"));
      break;
  }

  m_flag.Render();

  if (m_numPassengers > 0)
  {
    char caption[16];
    sprintf(caption, "%d", m_numPassengers);
    m_flag.RenderText(2, 2, caption);
  }

  //
  // If we are about to deploy, render a flag at the target

  if (m_conversionPoint != g_zeroVector)
  {
    LegacyVector3 front(0, 0, -1);
    front.RotateAroundY(g_gameTime * 0.5f);
    LegacyVector3 up = g_upVector;
    up.RotateAround(front * sinf(timeIndex * 3) * 0.3f);

    m_deployFlag.SetPosition(m_conversionPoint);
    m_deployFlag.SetOrientation(front, up);
    m_deployFlag.SetSize(30.0f);
    m_deployFlag.SetTexture(Resource::GetTexture("icons\\banner_deploy.bmp"));
    m_deployFlag.Render();
  }
}
