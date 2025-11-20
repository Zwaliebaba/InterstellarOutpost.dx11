#include "pch.h"
#include "lasertrooper.h"
#include "GameApp.h"
#include "camera.h"
#include "location.h"
#include "math_utils.h"
#include "team.h"
#include "unit.h"

void LaserTrooper::Begin()
{
  Entity::Begin();

  m_victoryDance = -1.0f;
}

bool LaserTrooper::Advance(Unit* _unit)
{
  if (m_targetPos == g_zeroVector)
    m_targetPos = m_pos;

  if (m_enabled && m_onGround && !m_dead)
  {
    LegacyVector3 movementDir = (m_targetPos - m_pos).Normalize();
    float distance = (m_targetPos - m_pos).Mag();
    float speed = m_stats[StatSpeed];
    if (speed * SERVER_ADVANCE_PERIOD > distance)
      speed = distance / SERVER_ADVANCE_PERIOD;
    m_vel = movementDir * speed;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_front = m_vel;
    m_front.Normalize();

    if (EnterTeleports())
      return true;
  }

  if (!m_onGround)
    AdvanceInAir(_unit);
  if (m_inWater != -1)
    AdvanceInWater(_unit);

  if (m_reloading > 0.0f)
  {
    m_reloading -= SERVER_ADVANCE_PERIOD;
    if (m_reloading < 0.0f)
      m_reloading = 0.0f;
  }

  if (m_dead)
  {
    bool amIDead = AdvanceDead(_unit);
    if (amIDead)
      return true;
  }

  if (m_victoryDance != -1.0f) { AdvanceVictoryDance(); }

  _unit->UpdateEntityPosition(m_pos, m_radius);

  return false;
}

void LaserTrooper::AdvanceVictoryDance()
{
  if (syncfrand(100.0f) < 1.0f)
  {
    m_vel.Zero();
    m_vel.y += 10.0f + syncfrand(10.0f);
    m_onGround = false;
  }
}

void LaserTrooper::Render(float predictionTime, int teamId)
{
  if (!m_enabled)
    return;

  //
  // Work out our predicted position and orientation

  LegacyVector3 predictedPos = m_pos + m_vel * predictionTime;
  if (m_onGround && m_inWater == -1)
  {
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);
  }

  float size = 2.0f;

  LegacyVector3 entityUp = g_app->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);
  //	float wobble = m_wobble;
  //    if( !m_onGround ) wobble = 0.0f;
  //	if ( m_vel.Mag() > 0.01f )
  //	{
  //		wobble += predictionTime * 15.0f;
  //	}
  //	entityUp.FastRotateAround(m_front, sinf(wobble) * 0.1f);
  entityUp.Normalize();
  LegacyVector3 entityFront = m_front;
  entityFront.RotateAround(m_angVel * predictionTime);

  //	Matrix34 transform(entityFront, entityUp, predictedPos);
  //	Shape *aShape = Resource::GetShape("laser_troop.shp");
  //	aShape->Render(predictionTime, transform);

  LegacyVector3 entityRight = entityFront ^ entityUp;
  entityFront *= 0.2f;
  entityUp *= size * 2.0f;
  entityRight.SetLength(size);

  if (m_justFired) { m_justFired = false; }

  //glDisable( GL_TEXTURE_2D );
  //RenderSphere( m_targetPos, 1.0f, RGBAColor(255,255,255,100) );
  //RenderSphere( m_unitTargetPos, 1.1f, RGBAColor(255,155,155,200) );
  //glEnable( GL_TEXTURE_2D );

  if (!m_dead)
  {
    RGBAColor colour = g_app->m_location->m_teams[teamId].m_colour;

    if (m_reloading > 0.0f)
    {
      colour.r *= 0.7f;
      colour.g *= 0.7f;
      colour.b *= 0.7f;
    }

    //
    // Draw our texture

    float maxHealth = EntityBlueprint::GetStat(m_entityType, StatHealth);
    float health = (float)m_stats[StatHealth] / maxHealth;
    if (health > 1.0f)
      health = 1.0f;

    colour *= 0.5f + 0.5f * health;
    glColor3ubv(colour.GetData());
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3fv((predictedPos - entityRight + entityUp).GetData());
    glTexCoord2f(1.0f, 1.0f);
    glVertex3fv((predictedPos + entityRight + entityUp).GetData());
    glTexCoord2f(1.0f, 0.0f);
    glVertex3fv((predictedPos + entityRight).GetData());
    glTexCoord2f(0.0f, 0.0f);
    glVertex3fv((predictedPos - entityRight).GetData());
    glEnd();

    //
    // Draw our shadow on the landscape

    if (m_onGround)
    {
      glColor4ub(0, 0, 0, 90);
      LegacyVector3 pos1 = predictedPos - entityRight;
      LegacyVector3 pos2 = predictedPos + entityRight;
      LegacyVector3 pos4 = pos1 + LegacyVector3(0.0f, 0.0f, size * 2.0f);
      LegacyVector3 pos3 = pos2 + LegacyVector3(0.0f, 0.0f, size * 2.0f);

      pos1.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue(pos1.x, pos1.z);
      pos2.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue(pos2.x, pos2.z);
      pos3.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue(pos3.x, pos3.z);
      pos4.y = 0.2f + g_app->m_location->m_landscape.m_heightMap->GetValue(pos4.x, pos4.z);

      glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f);
      glVertex3fv(pos1.GetData());
      glTexCoord2f(1.0f, 0.0f);
      glVertex3fv(pos2.GetData());
      glTexCoord2f(1.0f, 1.0f);
      glVertex3fv(pos3.GetData());
      glTexCoord2f(0.0f, 1.0f);
      glVertex3fv(pos4.GetData());
      glEnd();
    }

    //
    // Draw a line through us if we are side-on with the camera

    float alpha = 1.0f - fabsf(g_app->m_camera->GetFront() * m_front);
    if (alpha > 0.5f)
    {
      //colour.a = 255;
      colour.a = 255 * alpha;
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glColor4ubv(colour.GetData());
      glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 1.0f);
      glVertex3fv((predictedPos - entityFront * 1.5f + entityUp).GetData());
      glTexCoord2f(1.0f, 1.0f);
      glVertex3fv((predictedPos + entityFront * 1.5f + entityUp).GetData());
      glTexCoord2f(1.0f, 0.0f);
      glVertex3fv((predictedPos + entityFront * 1.5f).GetData());
      glTexCoord2f(0.0f, 0.0f);
      glVertex3fv((predictedPos - entityFront * 1.5f).GetData());
      glEnd();
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
  }
  else
  {
    entityFront = m_front;
    entityFront.Normalize();
    entityUp = g_upVector;
    entityRight = entityFront ^ entityUp;
    entityUp *= size * 2.0f;
    entityRight.Normalize();
    entityRight *= size;
    unsigned char alpha = (float)m_stats[StatHealth] * 2.55f;

    glColor4ub(0, 0, 0, alpha);

    entityRight *= 0.5f;
    entityUp *= 0.5;
    float predictedHealth = m_stats[StatHealth];
    if (m_onGround)
      predictedHealth -= 40 * predictionTime;
    else
      predictedHealth -= 20 * predictionTime;
    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);

    for (int i = 0; i < 3; ++i)
    {
      LegacyVector3 fragmentPos = predictedPos;
      if (i == 0)
        fragmentPos.x += 10.0f - predictedHealth / 10.0f;
      if (i == 1)
        fragmentPos.z += 10.0f - predictedHealth / 10.0f;
      if (i == 2)
        fragmentPos.x -= 10.0f - predictedHealth / 10.0f;
      fragmentPos.y += (fragmentPos.y - landHeight) * i * 0.5f;

      float left = 0.0f;
      float right = 1.0f;
      float top = 1.0f;
      float bottom = 0.0f;

      if (i == 0)
      {
        right -= (right - left) / 2;
        bottom -= (bottom - top) / 2;
      }
      if (i == 1)
      {
        left += (right - left) / 2;
        bottom -= (bottom - top) / 2;
      }
      if (i == 2)
      {
        top += (bottom - top) / 2;
        left += (right - left) / 2;
      }

      glBegin(GL_QUADS);
      glTexCoord2f(left, bottom);
      glVertex3fv((fragmentPos - entityRight + entityUp).GetData());
      glTexCoord2f(right, bottom);
      glVertex3fv((fragmentPos + entityRight + entityUp).GetData());
      glTexCoord2f(right, top);
      glVertex3fv((fragmentPos + entityRight).GetData());
      glTexCoord2f(left, top);
      glVertex3fv((fragmentPos - entityRight).GetData());
      glEnd();
    }
  }
}
