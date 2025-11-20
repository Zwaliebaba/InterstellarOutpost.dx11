#include "pch.h"
#include "wall.h"
#include "GameApp.h"
#include "location.h"
#include "math_utils.h"
#include "ParticleSystem.h"
#include "resource.h"
#include "shape.h"

Wall::Wall()
  : Building(BuildingType::TypeWall),
    m_damage(0.0f),
    m_fallSpeed(0.0f)
{
  Building::SetShape(Resource::GetShape("wall.shp"));
}

bool Wall::Advance()
{
  float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  float targetY = landHeight + m_damage / 10 + 0.01f;

  if (m_pos.y > targetY)
  {
    m_fallSpeed += 10.0f * SERVER_ADVANCE_PERIOD;
    m_pos.y -= m_fallSpeed * SERVER_ADVANCE_PERIOD;

    if (m_pos.y < targetY)
    {
      m_fallSpeed = 0.0f;
      m_pos.y = targetY;

      LegacyVector3 right = m_front ^ g_upVector;
      for (int i = -5; i < 5; ++i)
      {
        LegacyVector3 particlePos = m_pos + right * i * frand(10.0f);
        particlePos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(particlePos.x, particlePos.z) + 10.0f;
        LegacyVector3 particleVel(sfrand(10.0f), frand(10.0f), sfrand(10.0f));

        g_app->m_particleSystem->CreateParticle(particlePos, particleVel, Particle::TypeRocketTrail, 50.0f);
      }
    }
  }

  return false;
}

void Wall::Damage(float _damage) { m_damage -= _damage; }

void Wall::Render(float _predictionTime)
{
  LegacyVector3 predictedPos = m_pos - LegacyVector3(0, m_fallSpeed, 0) * _predictionTime;
  Matrix34 mat(m_front, g_upVector, predictedPos);
  m_shape->Render(_predictionTime, mat);
}
