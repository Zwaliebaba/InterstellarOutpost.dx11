#include "pch.h"
#include "entity_leg.h"
#include "GameApp.h"
#include "entity.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "Renderer.h"
#include "resource.h"
#include "shape.h"

EntityLeg::EntityLeg(const int _legNum, Entity* _parent, const char* _shapeNameUpper, const char* _shapeNameLower,
                     const char* _rootMarkerName)
  : m_legLift(0),
    m_idealLegSlope(0),
    m_legSwingDuration(0),
    m_lookAheadCoef(0),
    m_legNum(_legNum),
    m_parent(_parent)
{
  m_shapeUpper = Resource::GetShape(_shapeNameUpper);
  m_shapeLower = Resource::GetShape(_shapeNameLower);
  ASSERT_TEXT(m_shapeUpper, "EntityLeg: Couldn't load leg shape {}", _shapeNameUpper);
  ASSERT_TEXT(m_shapeLower, "EntityLeg: Couldn't load leg shape {}", _shapeNameLower);

  ShapeMarker* endMarker = m_shapeUpper->m_rootFragment->LookupMarker("MarkerEnd");
  const Matrix34& endMatrix = endMarker->GetWorldMatrix(Matrix34(0));
  m_thighLen = endMatrix.pos.Mag();

  endMarker = m_shapeLower->m_rootFragment->LookupMarker("MarkerEnd");
  const Matrix34& endMatrixLower = endMarker->GetWorldMatrix(Matrix34(0));
  m_shinLen = endMatrixLower.pos.Mag();

  m_rootMarker = m_parent->m_shape->m_rootFragment->LookupMarker(_rootMarkerName);
  ASSERT_TEXT(m_rootMarker, "EntityLeg: Couldn't find root marker %s", _rootMarkerName);

  m_foot.m_state = EntityFoot::OnGround;
}

LegacyVector3 EntityLeg::GetLegRootPos()
{
  Matrix34 rootMat(m_parent->m_front, g_upVector, m_parent->m_pos);
  const Matrix34& resultMat = m_rootMarker->GetWorldMatrix(rootMat);

  return resultMat.pos;
}

LegacyVector3 EntityLeg::CalcFootHomePos(const float _targetHoverHeight)
{
  LegacyVector3 rootWorldPos = GetLegRootPos();

  LegacyVector3 fromCenterToRoot = rootWorldPos - m_parent->m_pos;
  fromCenterToRoot.HorizontalAndNormalize();

  LegacyVector3 groundNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(m_parent->m_pos.x, m_parent->m_pos.z);
  fromCenterToRoot *= groundNormal.y;

  LegacyVector3 returnVal = rootWorldPos;
  returnVal += fromCenterToRoot * (m_idealLegSlope * _targetHoverHeight);
  returnVal.y -= _targetHoverHeight;

  return returnVal;
}

float EntityLeg::CalcFootsDesireToMove(const float _targetHoverHeight)
{
  LegacyVector3 homePos = CalcFootHomePos(_targetHoverHeight);
  LegacyVector3 delta = m_foot.m_pos - homePos;
  LegacyVector3 deltaHoriNorm = delta;
  deltaHoriNorm.HorizontalAndNormalize();

  float scoreDueToDirection = 1.0f;
  //	if (m_parent->m_vel.Mag() > 0.1f)
  //	{
  //		LegacyVector3 velHoriNorm = m_parent->m_vel;
  //		velHoriNorm.HorizontalAndNormalize();
  //		scoreDueToDirection = -(deltaHoriNorm * velHoriNorm);
  //	}
  float scoreDueToDist = delta.Mag();
  float score = scoreDueToDirection * scoreDueToDist;

  return score;
}

LegacyVector3 EntityLeg::CalcDesiredFootPos(const float _targetHoverHeight)
{
  LegacyVector3 rv = CalcFootHomePos(_targetHoverHeight);

  float expectedRotation = 1.2f * m_parent->m_angVel.y;
  LegacyVector3 averageExpectedVel = m_parent->m_vel;
  averageExpectedVel.RotateAroundY(expectedRotation * 0.5f);
  rv += averageExpectedVel * m_lookAheadCoef;

  return rv;
}

LegacyVector3 EntityLeg::CalcKneePos(const LegacyVector3& _footPos, const LegacyVector3& _rootPos, const LegacyVector3& _centerPos)
{
  LegacyVector3 rootToFoot(_footPos - _rootPos);
  float rootToFootLen = rootToFoot.Mag();

  LegacyVector3 rootToFootHoriNorm(rootToFoot);
  rootToFootHoriNorm.HorizontalAndNormalize();
  LegacyVector3 centerToRoot(_rootPos - _centerPos);
  centerToRoot.HorizontalAndNormalize();
  LegacyVector3 axis((centerToRoot ^ g_upVector).Normalize());
  float cosTheta = (rootToFootLen * 0.4f) / m_thighLen;
  // FIXME
  // cosTheta should never be greater than one, yet sometimes it is
  if (cosTheta > 1.0f)
    cosTheta = 1.0f;
  float theta = -acosf(cosTheta);

  LegacyVector3 footToKnee(-rootToFoot);
  footToKnee.SetLength(m_shinLen);
  footToKnee.RotateAround(axis * theta);

  return _footPos + footToKnee;
}

void EntityLeg::LiftFoot(const float _targetHoverHeight)
{
  m_foot.m_targetPos = CalcDesiredFootPos(_targetHoverHeight);
  m_foot.m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_foot.m_targetPos.x, m_foot.m_targetPos.z);
  m_foot.m_state = EntityFoot::Swinging;
  m_foot.m_leftGroundTimeStamp = g_gameTime;
  m_foot.m_lastGroundPos = m_foot.m_pos;
}

void EntityLeg::PlantFoot()
{
  m_foot.m_pos = m_foot.m_targetPos;
  m_foot.m_state = EntityFoot::OnGround;
}

LegacyVector3 EntityLeg::GetIdealSwingingFootPos(const float _fractionComplete)
{
  float fractionIncomplete = 1.0f - _fractionComplete;
  LegacyVector3 pos = m_foot.m_lastGroundPos * fractionIncomplete + m_foot.m_targetPos * _fractionComplete;

  if (_fractionComplete < 0.33f)
    pos.y += _fractionComplete * 3.0f * m_legLift;
  else if (fractionIncomplete < 0.33f)
    pos.y += fractionIncomplete * 3.0f * m_legLift;
  else
    pos.y += m_legLift;

  return pos;
}

// Returns true if the foot was planted this frame
bool EntityLeg::Advance()
{
  if (m_foot.m_state == EntityFoot::Swinging)
  {
    float fractionComplete = RampUpAndDown(m_foot.m_leftGroundTimeStamp, m_legSwingDuration, g_gameTime);
    if (fractionComplete > 1.0f)
    {
      PlantFoot();
      return true;
    }
    m_foot.m_pos = GetIdealSwingingFootPos(fractionComplete);
  }

  return false;
}

void EntityLeg::AdvanceSpiderPounce(const float _fractionComplete)
{
  LegacyVector3 offset = m_foot.m_bodyToFoot;
  if (_fractionComplete < 0.5f)
    offset *= 1.0f - _fractionComplete;
  else
    offset *= _fractionComplete;
  m_foot.m_pos = m_parent->m_pos - offset + m_parent->m_vel * _fractionComplete * 0.05f;
  m_foot.m_lastGroundPos = m_foot.m_pos;
  m_foot.m_targetPos = m_foot.m_pos;
}

void EntityLeg::Render(const float _predictionTime, const LegacyVector3& _predictedMovement)
{
  LegacyVector3 predictedPos = m_parent->m_pos + _predictedMovement;
  LegacyVector3 rootPos(_predictedMovement + GetLegRootPos());
  LegacyVector3 footPos;

  switch (m_foot.m_state)
  {
    case EntityFoot::OnGround:
      footPos = m_foot.m_pos;
      break;

    case EntityFoot::Swinging:
    {
      float fractionComplete = RampUpAndDown(m_foot.m_leftGroundTimeStamp, m_legSwingDuration, g_gameTime);
      footPos = GetIdealSwingingFootPos(fractionComplete);
      break;
    }

    case EntityFoot::Pouncing:
      footPos = m_foot.m_pos + _predictedMovement;
      //footPos = predictedPos - m_foot.m_bodyToFoot;
      break;
  }

  LegacyVector3 kneePos = CalcKneePos(footPos, rootPos, predictedPos);

  {
    LegacyVector3 up((kneePos - footPos).Normalize());
    LegacyVector3 front(up ^ g_upVector);
    Matrix34 mat(front, up, footPos);
    m_shapeLower->Render(_predictionTime, mat);
  }

  {
    LegacyVector3 up((rootPos - kneePos).Normalize());
    LegacyVector3 front(up ^ g_upVector);
    Matrix34 mat(front, up, kneePos);
    m_shapeUpper->Render(_predictionTime, mat);
  }
}
