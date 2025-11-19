#include "pch.h"
#include "weapons.h"
#include "Effect.h"
#include "GameApp.h"
#include "LegacyVector2.h"
#include "airstrike.h"
#include "camera.h"
#include "darwinian.h"
#include "entity_grid.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "insertion_squad.h"
#include "location.h"
#include "math_utils.h"
#include "obstruction_grid.h"
#include "ParticleSystem.h"
#include "Renderer.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"
#include "taskmanager.h"
#include "team.h"
#include "unit.h"

ThrowableWeapon::ThrowableWeapon(EffectType _type, const LegacyVector3& _startPos, const LegacyVector3& _front, float _force)
  : Effect(_type),
    m_shape(nullptr),
    m_force(1.0f),
    m_numFlashes(0)
{
  m_shape = Resource::GetShape("throwable.shp");
  m_pos = _startPos;
  m_vel = _front * _force;

  m_up = _front;
  m_front = g_upVector;
  LegacyVector3 right = m_front ^ m_up;
  m_front = right ^ m_up;

  m_birthTime = GetHighResTime();
}

void ThrowableWeapon::Initialize() { TriggerSoundEvent("Create"); }

void ThrowableWeapon::TriggerSoundEvent(const char* _event)
{
  switch (m_effectType)
  {
    case EffectType::EffectThrowableGrenade:
    case EffectType::EffectThrowableAirstrikeMarker:
    case EffectType::EffectThrowableControllerGrenade:
      g_app->m_soundSystem->TriggerOtherEvent(this, _event, SoundSourceBlueprint::TypeGrenade);
      break;
    case EffectType::EffectThrowableAirstrikeBomb:
      g_app->m_soundSystem->TriggerOtherEvent(this, _event, SoundSourceBlueprint::TypeAirstrikeBomb);
      break;
  }
}

bool ThrowableWeapon::Advance()
{
  if (m_effectType != EffectType::EffectThrowableAirstrikeMarker && syncfrand() > 0.2f)
  {
    LegacyVector3 vel(m_vel / 4.0f);
    vel.x += syncsfrand(5.0f);
    vel.y += syncsfrand(5.0f);
    vel.z += syncsfrand(5.0f);
    g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeRocketTrail, 40.0f);
  }

  if (m_force > 0.1f)
  {
    m_vel.y -= 9.8f;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    if (m_pos.y < landHeight + 1.0f)
    {
      BounceOffLandscape();
      m_force *= 0.75f;
      m_vel *= m_force;
      if (m_pos.y < landHeight + 1.0f)
        m_pos.y = landHeight + 1.0f;
      TriggerSoundEvent("Bounce");
    }
  }

  return false;
}

void ThrowableWeapon::Render(float _predictionTime)
{
  _predictionTime -= SERVER_ADVANCE_PERIOD;

  LegacyVector3 right = m_up ^ m_front;
  m_up.RotateAround(right * m_force * m_force * 0.15f);
  m_front = right ^ m_up;
  m_front.Normalize();
  m_up.Normalize();

  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
  Matrix34 transform(m_front, m_up, predictedPos);

  g_app->m_renderer->SetObjectLighting();
  glEnable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);

  m_shape->Render(_predictionTime, transform);

  glEnable(GL_BLEND);
  glDisable(GL_COLOR_MATERIAL);
  g_app->m_renderer->UnsetObjectLighting();

  int numFlashes = static_cast<int>(GetHighResTime() - m_birthTime);
  if (numFlashes > m_numFlashes)
  {
    TriggerSoundEvent("Flash");
    m_numFlashes = numFlashes;
  }

  float flashAlpha = 1.0f - ((GetHighResTime() - m_birthTime) - numFlashes);
  if (flashAlpha < 0.2f)
  {
    float distToThrowable = (g_app->m_camera->GetPos() - predictedPos).Mag();

    float size = 1000.0f / sqrtf(distToThrowable);
    glColor4ub(m_colour.r, m_colour.g, m_colour.b, 200);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
    glDisable(GL_CULL_FACE);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_app->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_app->m_camera->GetRight() * size).GetData());
    glEnd();
    size *= 0.4f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_app->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_app->m_camera->GetRight() * size).GetData());
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
  }
}

float ThrowableWeapon::GetMaxForce(int _researchLevel)
{
  switch (_researchLevel)
  {
    case 0:
      return 50.0f;
    case 1:
      return 80.0f;
    case 2:
      return 110.0f;
    case 3:
      return 140.0f;
    case 4:
      return 170.0f;
    default:
      return 0.0f;
  }
}

float ThrowableWeapon::GetApproxMaxRange(float _maxForce) { return _maxForce + (_maxForce * 0.75f) + (_maxForce * 0.75f * 0.75f); }

// ****************************************************************************
//  Class Grenade
// ****************************************************************************

Grenade::Grenade(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force)
  : ThrowableWeapon(EffectType::EffectThrowableGrenade, _startPos, _front, _force),
    m_life(3.0f),
    m_power(25.0f) { m_colour.Set(255, 100, 100); }

bool Grenade::Advance()
{
  m_life -= SERVER_ADVANCE_PERIOD;

  if (m_life <= 0.0f)
  {
    TriggerSoundEvent("Explode");

    g_app->m_location->Bang(m_pos, m_power / 2.0f, m_power * 2.0f);
    return true;
  }

  return ThrowableWeapon::Advance();
}

// ****************************************************************************
//  Class AirStrikeMarker
// ****************************************************************************

AirStrikeMarker::AirStrikeMarker(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force)
  : ThrowableWeapon(EffectType::EffectThrowableAirstrikeMarker, _startPos, _front, _force) { m_colour.Set(255, 100, 100); }

bool AirStrikeMarker::Advance()
{
  ThrowableWeapon::Advance();

  //
  // Dump loads of smoke for the bombers to see

  LegacyVector3 vel(m_vel / 4.0f);
  vel.x += syncsfrand(5.0f);
  vel.y += syncsfrand(5.0f);
  vel.z += syncsfrand(5.0f);
  g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeFire, 100.0f);

  //
  // If its time, summon the actual airstrike

  if (GetHighResTime() > m_birthTime + 2.0f)
  {
    if (m_airstrikeUnit.GetTeamId() != -1 && m_airstrikeUnit.GetUnitId() != -1)
    {
      // Air strike unit has been created
      Unit* unit = g_app->m_location->GetUnit(m_airstrikeUnit);
      if (!unit)
      {
        m_airstrikeUnit.SetInvalid();
        return true;
      }
      auto airStrikeUnit = static_cast<AirstrikeUnit*>(unit);
      if (airStrikeUnit->m_state == AirstrikeUnit::StateLeaving)
      {
        m_airstrikeUnit.SetInvalid();
        return true;
      }
    }
    else
    {
      // Summon an air strike now
      int teamId = g_app->m_globalWorld->m_myTeamId;
      int unitId;
      Team* team = g_app->m_location->GetMyTeam();

      int airStikeResearch = g_app->m_globalWorld->m_research->CurrentLevel(GlobalResearch::TypeAirStrike);
      auto unit = static_cast<AirstrikeUnit*>(team->NewUnit(EntityType::TypeSpaceInvader, airStikeResearch, &unitId, m_pos));
      unit->m_effectId = m_id.GetIndex();
      m_airstrikeUnit.Set(teamId, unitId, -1, -1);
    }
  }

  return false;
}

// ****************************************************************************
//  Class ControllerGrenade
// ****************************************************************************

ControllerGrenade::ControllerGrenade(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force)
  : ThrowableWeapon(EffectType::EffectThrowableControllerGrenade, _startPos, _front, _force) { m_colour.Set(100, 100, 255); }

bool ControllerGrenade::Advance()
{
  ThrowableWeapon::Advance();

  if (GetHighResTime() > m_birthTime + 3.0f)
  {
    g_app->m_soundSystem->TriggerOtherEvent(this, "ExplodeController", SoundSourceBlueprint::TypeGrenade);

    int numFlashes = 5 + darwiniaRandom() % 5;
    for (int i = 0; i < numFlashes; ++i)
    {
      LegacyVector3 vel(sfrand(15.0f), frand(35.0f), sfrand(15.0f));
      g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeControlFlash, 100.0f);
    }

    Task* currentTask = g_app->m_taskManager->GetCurrentTask();
    if (currentTask && currentTask->m_type == GlobalResearch::TypeSquad)
    {
      Unit* owner = g_app->m_location->GetUnit(currentTask->m_objId);
      if (owner && owner->m_troopType == EntityType::TypeSquadie)
      {
        auto squad = static_cast<InsertionSquad*>(owner);

        int numFound;
        WorldObjectId* ids = g_app->m_location->m_entityGrid->GetFriends(m_pos.x, m_pos.z, 50.0f, &numFound, m_id.GetTeamId());
        for (int i = 0; i < numFound; ++i)
        {
          WorldObjectId id = ids[i];
          Entity* entity = g_app->m_location->GetEntity(id);
          if (entity && entity->m_entityType == EntityType::TypeDarwinian)
          {
            auto darwinian = dynamic_cast<Darwinian*>(entity);
            darwinian->TakeControl(squad->m_controllerId);
          }
        }
      }
    }

    return true;
  }

  return false;
}

Rocket::Rocket(LegacyVector3 _startPos, LegacyVector3 _targetPos)
  : Effect(EffectType::EffectRocket),
    m_fromTeamId(255),
    m_target(_targetPos)
{
  m_pos = _startPos + LegacyVector3(0, 2, 0);
  m_vel = (_targetPos - m_pos).Normalize() * 50.0f;

  m_shape = Resource::GetShape("throwable.shp");

  m_timer = GetHighResTime();
}

void Rocket::Initialize() { g_app->m_soundSystem->TriggerOtherEvent(this, "Create", SoundSourceBlueprint::TypeRocket); }

bool Rocket::Advance()
{
  //
  // Move a little bit

  if (m_vel.Mag() < 150.0f)
    m_vel *= 1.2f;

  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  //
  // Create smoke trail

  if (syncfrand() > 0.2f)
  {
    LegacyVector3 vel(m_vel / 10.0f);
    vel.x += syncsfrand(4.0f);
    vel.y += syncsfrand(4.0f);
    vel.z += syncsfrand(4.0f);
    g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeFire);
  }

  //
  // Have we run out of steam?

  int rocketResearch = g_app->m_globalWorld->m_research->CurrentLevel(GlobalResearch::TypeRocket);
  float maxLife = 0.0f;
  switch (rocketResearch)
  {
    case 0:
      maxLife = 0.25f;
      break;
    case 1:
      maxLife = 0.5f;
      break;
    case 2:
      maxLife = 0.75f;
      break;
    case 3:
      maxLife = 1.0f;
      break;
    case 4:
      maxLife = 1.5f;
      break;
  }

  if (GetHighResTime() > m_timer + maxLife)
  {
    g_app->m_location->Bang(m_pos, 15.0f, 25.0f);
    g_app->m_soundSystem->StopAllSounds(m_id, "Rocket Create");
    g_app->m_soundSystem->TriggerOtherEvent(this, "Explode", SoundSourceBlueprint::TypeRocket);
    return true;
  }

  //
  // Have we hit the ground?

  if (g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) >= m_pos.y)
  {
    g_app->m_location->Bang(m_pos, 15.0f, 25.0f);
    g_app->m_soundSystem->TriggerOtherEvent(this, "Explode", SoundSourceBlueprint::TypeRocket);
    return true;
  }

  //
  // Have we hit any buildings?

  LList<int>* buildings = g_app->m_location->m_obstructionGrid->GetBuildings(m_pos.x, m_pos.z);

  for (int b = 0; b < buildings->Size(); ++b)
  {
    int buildingId = buildings->GetData(b);
    Building* building = g_app->m_location->GetBuilding(buildingId);
    if (building->DoesSphereHit(m_pos, 3.0f))
    {
      g_app->m_location->Bang(m_pos, 15.0f, 25.0f);
      g_app->m_soundSystem->TriggerOtherEvent(this, "Explode", SoundSourceBlueprint::TypeRocket);
      return true;
    }
  }

  return false;
}

void Rocket::Render(float predictionTime)
{
  LegacyVector3 predictedPos = m_pos + m_vel * predictionTime;

  LegacyVector3 front = m_vel;
  front.Normalize();
  LegacyVector3 right = front ^ g_upVector;
  LegacyVector3 up = right ^ front;

  Matrix34 transform(front, up, predictedPos);

  g_app->m_renderer->SetObjectLighting();
  glEnable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);

  m_shape->Render(predictionTime, transform);

  glEnable(GL_BLEND);
  glDisable(GL_COLOR_MATERIAL);
  g_app->m_renderer->UnsetObjectLighting();

  for (int i = 0; i < 5; ++i)
  {
    LegacyVector3 vel(m_vel / -20.0f);
    vel.x += sfrand(8.0f);
    vel.y += sfrand(8.0f);
    vel.z += sfrand(8.0f);
    LegacyVector3 pos = predictedPos - m_vel * (0.05f + frand(0.05f));
    pos.x += sfrand(8.0f);
    pos.y += sfrand(8.0f);
    pos.z += sfrand(8.0f);
    float size = 50.0f + frand(50.0f);
    g_app->m_particleSystem->CreateParticle(pos, vel, Particle::TypeMissileFire, size);
  }
}

// ****************************************************************************
//  Class Laser
// ****************************************************************************

void Laser::Initialize(float _lifeTime)
{
  m_life = _lifeTime;
  m_harmless = false;
  m_bounced = false;

  g_app->m_soundSystem->TriggerOtherEvent(this, "Create", SoundSourceBlueprint::TypeLaser);
}

bool Laser::Advance()
{
  m_life -= SERVER_ADVANCE_PERIOD;
  if (m_life <= 0.0f)
    return true;

  LegacyVector3 oldPos = m_pos;
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  //
  // Detect collisions with landscape / buildings / people

  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

  if (m_pos.y <= landHeight)
  {
    if (m_bounced)
    {
      // Second richochet, so die
      return true;
    }
    // Richochet
    LegacyVector3 hitPoint;
    LegacyVector3 vel = m_vel;
    vel.Normalize();
    g_app->m_location->m_landscape.RayHit(oldPos, vel, &hitPoint);
    float distanceTravelled = (hitPoint - oldPos).Mag();
    float distanceTotal = (m_vel * SERVER_ADVANCE_PERIOD).Mag();

    LegacyVector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(hitPoint.x, hitPoint.z);
    LegacyVector3 incomingVel = m_vel * -1.0f;
    float dotProd = normal * incomingVel;
    m_vel = 2.0f * dotProd * normal - incomingVel;

    m_pos = hitPoint;
    LegacyVector3 distanceRemaining = m_vel;
    distanceRemaining.SetLength(distanceTotal - distanceTravelled);

    m_pos += distanceRemaining;
    g_app->m_soundSystem->TriggerOtherEvent(this, "Richochet", SoundSourceBlueprint::TypeLaser);

    m_bounced = true;
  }
  else if (m_pos.x < 0 || m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() || m_pos.z < 0 || m_pos.z > g_app->m_location->
    m_landscape.GetWorldSizeZ())
  {
    // Outside game world
    return true;
  }
  else
  {
    //
    // Detect collisions with buildings

    LegacyVector3 rayDir = m_vel;
    rayDir.Normalize();
    LegacyVector3 hitPos(0, 0, 0);
    LegacyVector3 hitNorm(0, 0, 0);

    LList<int>* nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings(m_pos.x, m_pos.z);
    for (int i = 0; i < nearbyBuildings->Size(); ++i)
    {
      Building* building = g_app->m_location->GetBuilding(nearbyBuildings->GetData(i));
      if (building->DoesRayHit(m_pos, rayDir, (m_vel * SERVER_ADVANCE_PERIOD).Mag(), &hitPos, &hitNorm))
      {
        LegacyVector3 vel(-m_vel / 15.0f);
        vel.x += sfrand(10.0f);
        vel.y += sfrand(10.0f);
        vel.z += sfrand(10.0f);
        g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeRocketTrail);
        g_app->m_soundSystem->TriggerOtherEvent(this, "HitBuilding", SoundSourceBlueprint::TypeLaser);
        return true;
      }
    }

    //
    // Detect collisions with entities

    if (!m_harmless)
    {
      LegacyVector3 halfDelta = m_vel * (SERVER_ADVANCE_PERIOD * 0.5f);
      LegacyVector3 rayStart = m_pos - halfDelta;
      LegacyVector3 rayEnd = m_pos + halfDelta;
      int numFound;
      float maxRadius = halfDelta.Mag() * 2.0f;
      WorldObjectId* ids = g_app->m_location->m_entityGrid->GetEnemies(m_pos.x, m_pos.z, maxRadius, &numFound, m_fromTeamId);
      for (int i = 0; i < numFound; ++i)
      {
        WorldObjectId id = ids[i];
        WorldObject* wobj = g_app->m_location->GetEntity(id);
        auto entity = dynamic_cast<Entity*>(wobj);

        if (PointSegDist2D(LegacyVector2(entity->m_pos), LegacyVector2(rayStart), LegacyVector2(rayEnd)) < 10.0f)
        {
          g_app->m_soundSystem->TriggerOtherEvent(this, "HitEntity", SoundSourceBlueprint::TypeLaser);
          if (entity->m_entityType != EntityType::TypeSpider && entity->m_entityType != EntityType::TypeSporeGenerator && entity->
            m_entityType != EntityType::TypeEngineer && entity->m_entityType != EntityType::TypeTriffidEgg && entity->m_entityType
            != EntityType::TypeSoulDestroyer && entity->m_entityType != EntityType::TypeArmour)
          {
            float damage = 20.0f + syncfrand(20.0f);
            entity->ChangeHealth(static_cast<int>(-damage));
            LegacyVector3 push(m_vel);
            push.SetLength(syncfrand(10.0f));
            push.y = 7.0f + syncfrand(5.0f);
            if (entity->m_entityType != EntityType::TypeSquadie && entity->m_entityType != EntityType::TypeVirii)
            {
              entity->m_front = push;
              entity->m_front.Normalize();
            }
            if (entity->m_entityType != EntityType::TypeVirii)
              entity->m_vel += push;

            m_harmless = true;
          }
          return false;
        }
      }
    }
  }

  return false;
}

void Laser::Render(float predictionTime)
{
  LegacyVector3 predictedPos = m_pos + m_vel * predictionTime;

  //
  // No richochet occurred recently
  LegacyVector3 lengthVector = m_vel;
  lengthVector.SetLength(10.0f);
  LegacyVector3 fromPos = predictedPos;
  LegacyVector3 toPos = predictedPos - lengthVector;

  LegacyVector3 midPoint = fromPos + (toPos - fromPos) / 2.0f;
  LegacyVector3 camToMidPoint = g_app->m_camera->GetPos() - midPoint;
  float camDistSqd = camToMidPoint.MagSquared();
  LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalize();

  rightAngle *= 0.8f;

  const RGBAColor& colour = g_app->m_location->m_teams[m_fromTeamId].m_colour;
  glColor4ub(colour.r, colour.g, colour.b, 255);

  glBegin(GL_QUADS);
  for (int i = 0; i < 5; ++i)
  {
    glTexCoord2i(0, 0);
    glVertex3fv((fromPos - rightAngle).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((fromPos + rightAngle).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((toPos + rightAngle).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((toPos - rightAngle).GetData());
  }
  glEnd();

  if (camDistSqd < 200.0f)
    g_app->m_camera->CreateCameraShake(0.5f);
}

Shockwave::Shockwave(int _teamId, float _size)
  : Effect(EffectType::EffectShockwave),
    m_shape(nullptr),
    m_teamId(_teamId),
    m_size(_size),
    m_life(_size) {}

bool Shockwave::Advance()
{
  m_life -= SERVER_ADVANCE_PERIOD;
  if (m_life <= 0.0f)
    return true;

  float radius = 35.0f + 40.0f * (m_size - m_life);
  int numFound;
  WorldObjectId* ids;
  if (m_teamId != 255)
    ids = g_app->m_location->m_entityGrid->GetEnemies(m_pos.x, m_pos.z, radius, &numFound, m_teamId);
  else
    ids = g_app->m_location->m_entityGrid->GetNeighbours(m_pos.x, m_pos.z, radius, &numFound);

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = ids[i];
    WorldObject* wobj = g_app->m_location->GetEntity(id);
    auto ent = static_cast<Entity*>(wobj);
    float distance = (ent->m_pos - m_pos).Mag();
    if (fabs(distance - radius) < 10.0f)
    {
      float fraction = (m_life / m_size);
      if (ent->m_onGround)
      {
        LegacyVector3 push(ent->m_pos - m_pos);
        push.Normalize();
        push.y = 3.0f;
        push.SetLength(25.0f * fraction);
        ent->m_vel += push;
        ent->m_onGround = false;
      }
      if (ent->m_entityType == EntityType::TypeDarwinian)
      {
        if (syncfrand() < 0.1f)
          static_cast<Darwinian*>(ent)->SetFire();
        else
          ent->ChangeHealth(-40 * fraction);
      }
      else
        ent->ChangeHealth(-40 * fraction);
    }
  }

  return false;
}

void Shockwave::Render(float predictionTime)
{
  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);

  glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
  int numSteps = 50.0f;
  float predictedLife = m_life - predictionTime;
  float radius = 35.0f + 40.0f * (m_size - predictedLife);
  float alpha = 0.6f * predictedLife / m_size;

  glBegin(GL_TRIANGLE_FAN);
  glColor4f(1.0f, 1.0f, 0.0f, 0.0f);
  glVertex3fv((m_pos + LegacyVector3(0, 5, 0)).GetData());
  glColor4f(1.0f, 0.5f, 0.5f, alpha);

  float angle = 0.0f;
  for (int i = 0; i <= numSteps; ++i)
  {
    float xDiff = radius * sinf(angle);
    float zDiff = radius * cosf(angle);
    LegacyVector3 pos = m_pos + LegacyVector3(xDiff, 5, zDiff);
    glVertex3fv(pos.GetData());
    angle += 2.0f * M_PI / static_cast<float>(numSteps);
  }

  glEnd();

  glShadeModel(GL_FLAT);
  glEnable(GL_DEPTH_TEST);

  //
  // Render screen flash if we are new

  if (m_size - predictedLife < 0.1f)
  {
    if (g_app->m_camera->PosInViewFrustum(m_pos))
    {
      float distance = (g_app->m_camera->GetPos() - m_pos).Mag();
      float distanceFactor = 1.0f - (distance / 500.0f);
      if (distanceFactor < 0.0f)
        distanceFactor = 0.0f;

      float alpha = 1.0f - (m_size - predictedLife) / 0.1f;
      alpha *= distanceFactor;
      glColor4f(1, 1, 1, alpha);
      g_app->m_renderer->SetupMatricesFor2D();
      int screenW = ClientEngine::OutputSize().Width;
      int screenH = ClientEngine::OutputSize().Height;
      glDisable(GL_CULL_FACE);
      glBegin(GL_QUADS);
      glVertex2i(0, 0);
      glVertex2i(screenW, 0);
      glVertex2i(screenW, screenH);
      glVertex2i(0, screenH);
      glEnd();
      glEnable(GL_CULL_FACE);
      g_app->m_renderer->SetupMatricesFor3D();

      g_app->m_camera->CreateCameraShake(alpha);
    }
  }

  //
  // Render big blast

  if (m_size - predictedLife < 1.0f)
  {
    float distToBang = (g_app->m_camera->GetPos() - m_pos).Mag();
    LegacyVector3 predictedPos = m_pos;
    float size = (m_size * 2000.0f) / sqrtf(distToBang);
    float alpha = 1.0f - (m_size - predictedLife) / 1.0f;
    glColor4f(1.0f, 0.4f, 0.4f, alpha);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
    glDisable(GL_CULL_FACE);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_app->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_app->m_camera->GetRight() * size).GetData());
    glEnd();

    size *= 0.4f;
    glColor4f(1.0f, 1.0f, 0.0f, alpha);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + g_app->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + g_app->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - g_app->m_camera->GetRight() * size).GetData());
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
  }
}

// ****************************************************************************
// Class MuzzleFlash
// ****************************************************************************

MuzzleFlash::MuzzleFlash()
  : Effect(EffectType::EffectMuzzleFlash),
    m_size(1.0f),
    m_life(1.0f) {}

MuzzleFlash::MuzzleFlash(const LegacyVector3& _pos, const LegacyVector3& _front, float _size, float _life)
  : Effect(EffectType::EffectMuzzleFlash),
    m_front(_front),
    m_size(_size),
    m_life(_life) { m_pos = _pos; }

bool MuzzleFlash::Advance()
{
  if (m_life <= 0.0f)
    return true;

  m_pos += m_front * SERVER_ADVANCE_PERIOD * 10.0f;
  m_life -= SERVER_ADVANCE_PERIOD * 10.0f;

  return false;
}

void MuzzleFlash::Render(float _predictionTime)
{
  float predictedLife = m_life - _predictionTime * 10.0f;
  //float predictedLife = m_life;
  LegacyVector3 predictedPos = m_pos + m_front * _predictionTime * 10.0f;

  LegacyVector3 fromPos = predictedPos;
  LegacyVector3 toPos = predictedPos + m_front * m_size;

  LegacyVector3 midPoint = fromPos + (toPos - fromPos) / 2.0f;
  LegacyVector3 camToMidPoint = g_app->m_camera->GetPos() - midPoint;
  LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalize();
  LegacyVector3 toPosToFromPos = toPos - fromPos;

  rightAngle *= m_size * 0.5f;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\muzzleflash.bmp"));
  glDepthMask(false);

  float alpha = predictedLife;
  alpha = std::min(1.0f, alpha);
  alpha = std::max(0.0f, alpha);
  glColor4f(1.0f, 1.0f, 1.0f, alpha);

  glBegin(GL_QUADS);
  for (int i = 0; i < 5; ++i)
  {
    rightAngle *= 0.8f;
    toPosToFromPos *= 0.8f;
    glTexCoord2i(0, 0);
    glVertex3fv((fromPos - rightAngle).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((fromPos + rightAngle).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((fromPos + toPosToFromPos + rightAngle).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((fromPos + toPosToFromPos - rightAngle).GetData());
  }
  glEnd();

  glDisable(GL_TEXTURE_2D);
}

Missile::Missile()
  : Effect(EffectType::Missile)
{
  m_shape = Resource::GetShape("missile.shp");
  m_booster = m_shape->m_rootFragment->LookupMarker("MarkerBooster");

  int rocketResearch = g_app->m_globalWorld->m_research->CurrentLevel(GlobalResearch::TypeRocket);
  m_life = 5.0f + rocketResearch * 5.0f;
}

bool Missile::AdvanceToTargetPosition(const LegacyVector3& _pos)
{
  float amountToTurn = SERVER_ADVANCE_PERIOD * 2.0f;
  LegacyVector3 targetDir = (_pos - m_pos).Normalize();

  // Look ahead to see if we're about to hit the ground
  LegacyVector3 forwardPos = m_pos + targetDir * 100.0f;
  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(forwardPos.x, forwardPos.z);
  if (forwardPos.y <= landHeight && (forwardPos - _pos).Mag() > 100.0f)
    targetDir = g_upVector;

  LegacyVector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
  actualDir.Normalize();
  float speed = 200.0f;

  LegacyVector3 oldPos = m_pos;
  LegacyVector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
  landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(newPos.x, newPos.z);
  if (newPos.y <= landHeight)
    return true;

  LegacyVector3 moved = newPos - oldPos;
  if (moved.Mag() > speed * SERVER_ADVANCE_PERIOD)
    moved.SetLength(speed * SERVER_ADVANCE_PERIOD);
  newPos = m_pos + moved;

  m_pos = newPos;
  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
  m_front = actualDir;

  LegacyVector3 right = m_front ^ g_upVector;
  m_up = right ^ m_front;

  return (m_pos - _pos).Mag() < 20.0f;
}

bool Missile::Advance()
{
  m_life -= SERVER_ADVANCE_PERIOD;
  if (m_life < 0.0f)
  {
    Explode();
    return true;
  }

  bool arrived = AdvanceToTargetPosition(m_target);
  if (arrived)
  {
    Explode();
    return true;
  }

  m_history.PutData(m_pos);

  //
  // Create smoke trail

  LegacyVector3 vel(m_vel / -20.0f);
  vel.x += syncsfrand(2.0f);
  vel.y += syncsfrand(2.0f);
  vel.z += syncsfrand(2.0f);
  float size = 50.0f + syncfrand(150.0f);

  Matrix34 mat(m_front, m_up, m_pos);
  LegacyVector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;
  g_app->m_particleSystem->CreateParticle(boosterPos - m_vel * SERVER_ADVANCE_PERIOD * 2.0f, vel, Particle::TypeMissileTrail, size);
  g_app->m_particleSystem->CreateParticle(boosterPos - m_vel * SERVER_ADVANCE_PERIOD * 1.5f, vel, Particle::TypeMissileTrail, size);

  return false;
}

void Missile::Explode() { g_app->m_location->Bang(m_pos, 20.0f, 100.0f); }

void Missile::Render(float _predictionTime)
{
  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
  Matrix34 mat(m_front, m_up, predictedPos);

  glDisable(GL_BLEND);

  g_app->m_renderer->SetObjectLighting();
  glEnable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);

  m_shape->Render(_predictionTime, mat);
  g_app->m_renderer->UnsetObjectLighting();

  glEnable(GL_BLEND);
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_CULL_FACE);

  LegacyVector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;
  m_fire.m_pos = boosterPos;
  m_fire.m_vel = m_vel;
  m_fire.m_size = 30.0f + frand(20.0f);
  m_fire.m_front = -m_front;
  m_fire.Render(_predictionTime);

  for (int i = 0; i < 5; ++i)
  {
    LegacyVector3 vel(m_vel / -10.0f);
    vel.x += sfrand(8.0f);
    vel.y += sfrand(8.0f);
    vel.z += sfrand(8.0f);
    LegacyVector3 pos = predictedPos - m_vel * (0.1f + frand(0.1f));
    pos.x += sfrand(8.0f);
    pos.y += sfrand(8.0f);
    pos.z += sfrand(8.0f);
    float size = 200.0f + frand(200.0f);
    g_app->m_particleSystem->CreateParticle(pos, vel, Particle::TypeMissileFire, size);
  }
}

TurretShell::TurretShell(float _life)
  : Effect(EffectType::EffectGunTurretShell),
    m_life(_life) {}

bool TurretShell::Advance()
{
  //
  // Update our position

  LegacyVector3 oldPos = m_pos;

  m_life -= SERVER_ADVANCE_PERIOD;
  m_pos += m_vel * SERVER_ADVANCE_PERIOD;

  if (m_life <= 0.0f)
    return true;

  if (m_pos.x < 0 || m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() || m_pos.z < 0 || m_pos.z > g_app->m_location->
    m_landscape.GetWorldSizeZ() || m_pos.y < 0)
  {
    // Outside of world
    return true;
  }

  //
  // Did we hit anyone?

  LegacyVector3 centerPos = (m_pos + oldPos) / 2.0f;
  float radius = (m_pos - oldPos).Mag() / 1.0f;
  int numFound;
  WorldObjectId* ids = g_app->m_location->m_entityGrid->GetNeighbours(centerPos.x, centerPos.z, radius, &numFound);

  for (int i = 0; i < numFound; ++i)
  {
    WorldObjectId id = ids[i];
    Entity* entity = g_app->m_location->GetEntity(id);
    if (entity)
    {
      LegacyVector3 rayDir = m_vel;
      rayDir.Normalize();
      if (entity->RayHit(oldPos, rayDir))
      {
        entity->ChangeHealth(-10);
        if (entity->m_onGround)
        {
          LegacyVector3 push(entity->m_pos - m_pos);
          push.Normalize();
          push += LegacyVector3(syncsfrand(3.0f), syncfrand(3.0f), syncsfrand(3.0f));
          push.SetLength(20.0f);
          entity->m_vel += push;
          entity->m_onGround = false;
        }
      }
    }
  }

  //
  // Did we hit any buildings?

  {
    LegacyVector3 rayDir = m_vel;
    rayDir.Normalize();
    LegacyVector3 hitPos(0, 0, 0);
    LegacyVector3 hitNorm(0, 0, 0);

    for (int i = 0; i < g_app->m_location->m_buildings.Size(); ++i)
    {
      if (g_app->m_location->m_buildings.ValidIndex(i))
      {
        Building* building = g_app->m_location->m_buildings.GetData(i);
        if (building->DoesRayHit(m_pos, rayDir, (m_vel * SERVER_ADVANCE_PERIOD).Mag(), &hitPos, &hitNorm))
        {
          for (int p = 0; p < 3; ++p)
          {
            LegacyVector3 vel = (m_pos - building->m_centerPos).Normalize();
            vel *= 50.0f;
            vel.x += sfrand(10.0f);
            vel.y += frand(10.0f);
            vel.z += sfrand(10.0f);
            float size = 25.0f + frand(25.0f);
            g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeRocketTrail, size);
          }
          building->Damage(-2);
          return true;
        }
      }
    }
  }

  //
  // Did we hit the landscape?

  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

  if (m_pos.y <= landHeight)
  {
    for (int i = 0; i < 3; ++i)
    {
      LegacyVector3 vel = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
      vel *= 50.0f;
      vel.x += sfrand(10.0f);
      vel.y += frand(10.0f);
      vel.z += sfrand(10.0f);
      float size = 25.0f + frand(25.0f);
      g_app->m_particleSystem->CreateParticle(oldPos, vel, Particle::TypeRocketTrail, size);
    }

    return true;
  }

  return false;
}

void TurretShell::Render(float predictionTime)
{
  predictionTime += SERVER_ADVANCE_PERIOD;

  LegacyVector3 predictedPos = m_pos + m_vel * predictionTime;
  LegacyVector3 predictedFront = m_vel;
  predictedFront.Normalize();
  LegacyVector3 right = predictedFront ^ g_upVector;
  LegacyVector3 up = right ^ predictedFront;
  Shape* shape = Resource::GetShape("turretshell.shp");

  Matrix34 shellMat(predictedFront, up, predictedPos);

  glDisable(GL_BLEND);
  g_app->m_renderer->SetObjectLighting();
  shape->Render(predictionTime, shellMat);
  g_app->m_renderer->UnsetObjectLighting();
  glEnable(GL_BLEND);
}
