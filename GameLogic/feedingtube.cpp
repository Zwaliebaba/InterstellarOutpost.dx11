#include "pch.h"
#include "feedingtube.h"
#include "GameApp.h"
#include "camera.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "ogl_extensions.h"
#include "profiler.h"
#include "resource.h"
#include "shape.h"
#include "text_stream_readers.h"

FeedingTube::FeedingTube()
  : Building(BuildingType::TypeFeedingTube),
    m_receiverId(-1),
    m_range(0.0f),
    m_signal(0.0f)
{
  Building::SetShape(Resource::GetShape("feedingtube.shp"));
  m_focusMarker = m_shape->m_rootFragment->LookupMarker("MarkerFocus");
}

// *** Initialize
void FeedingTube::Initialize(Building* _template)
{
  Building::Initialize(_template);
  DEBUG_ASSERT(_template->m_buildingType == BuildingType::TypeFeedingTube);
  m_receiverId = static_cast<FeedingTube*>(_template)->m_receiverId;
}

bool FeedingTube::Advance()
{
  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
  LegacyVector3 dishPos = worldMat.pos;

  auto ft = static_cast<FeedingTube*>(g_app->m_location->GetBuilding(m_receiverId));
  if (ft && ft->m_buildingType == BuildingType::TypeFeedingTube)
    m_range = (ft->GetDishPos(0.0f) - dishPos).Mag();
  else
    m_range = 0.0f;

  return Building::Advance();
}

LegacyVector3 FeedingTube::GetDishPos(float _predictionTime)
{
  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
  return worldMat.pos;
}

LegacyVector3 FeedingTube::GetDishFront(float _predictionTime)
{
  if (m_receiverId != -1)
  {
    auto receiver = static_cast<FeedingTube*>(g_app->m_location->GetBuilding(m_receiverId));
    if (receiver)
    {
      LegacyVector3 ourDishPos = GetDishPos(_predictionTime);
      LegacyVector3 receiverDishPos = receiver->GetDishPos(_predictionTime);
      return (receiverDishPos - ourDishPos).Normalize();
    }
  }

  Matrix34 rootMat(m_front, g_upVector, m_pos);
  Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
  return worldMat.f;
}

LegacyVector3 FeedingTube::GetForwardsClippingDir(float _predictionTime, FeedingTube* _sender)
{
  if (_sender == nullptr)
    return GetDishFront(_predictionTime);

  LegacyVector3 senderDishFront = _sender->GetDishFront(_predictionTime);

  LegacyVector3 dishFront = GetDishFront(_predictionTime);

  // Make the two dishFronts point at each other.

  LegacyVector3 SR = GetDishPos(_predictionTime) - _sender->GetDishPos(_predictionTime);
  SR.Normalize();

  if (SR * senderDishFront < 0)
    senderDishFront *= -1;

  if (SR * dishFront > 0)
    dishFront *= -1;

  LegacyVector3 combinedDirection = -senderDishFront + dishFront;

  if (combinedDirection.MagSquared() < 0.5f)
    return -dishFront;

  return combinedDirection.Normalize();
}

void FeedingTube::Render(float _predictionTime) { Building::Render(_predictionTime); }

void FeedingTube::RenderAlphas(float _predictionTime)
{
  if (m_receiverId != -1)
  {
    RenderSignal(_predictionTime, 10.0f, 0.4f);
    RenderSignal(_predictionTime, 9.0f, 0.2f);
    RenderSignal(_predictionTime, 8.0f, 0.2f);
    RenderSignal(_predictionTime, 4.0f, 0.5f);
  }
}

void FeedingTube::RenderSignal(float _predictionTime, float _radius, float _alpha)
{
  START_PROFILE(g_app->m_profiler, "Signal");

  auto receiver = static_cast<FeedingTube*>(g_app->m_location->GetBuilding(m_receiverId));
  if (!receiver)
    return;

  LegacyVector3 startPos = GetStartPoint();
  LegacyVector3 endPos = GetEndPoint();
  LegacyVector3 delta = endPos - startPos;
  LegacyVector3 deltaNorm = delta;
  deltaNorm.Normalize();

  float distance = (startPos - endPos).Mag();
  float numRadii = 20.0f;
  int numSteps = static_cast<int>(distance / 200.0f);
  numSteps = std::max(1, numSteps);

  glEnable(GL_TEXTURE_2D);

  gglActiveTextureARB(GL_TEXTURE0_ARB);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laserfence.bmp", true, true));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);
  glEnable(GL_TEXTURE_2D);

  gglActiveTextureARB(GL_TEXTURE1_ARB);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\radarsignal.bmp", true, true));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
  glEnable(GL_TEXTURE_2D);

  glDisable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  glDepthMask(false);
  glColor4f(1.0f, 1.0f, 1.0f, _alpha);

  glMatrixMode(GL_MODELVIEW);
#ifdef USE_DIRECT3D
  void SwapToViewMatrix();
  void SwapToModelMatrix();

  SwapToViewMatrix();
#endif
  glTranslatef(startPos.x, startPos.y, startPos.z);
  LegacyVector3 dishFront = GetForwardsClippingDir(_predictionTime, receiver);
  float eqn1[4] = {dishFront.x, dishFront.y, dishFront.z, -1.0f};
  glClipPlane(GL_CLIP_PLANE0, eqn1);

  LegacyVector3 receiverPos = receiver->GetDishPos(_predictionTime);
  LegacyVector3 receiverFront = receiver->GetForwardsClippingDir(_predictionTime, this);
  glTranslatef(-startPos.x, -startPos.y, -startPos.z);
  glTranslatef(receiverPos.x, receiverPos.y, receiverPos.z);

  LegacyVector3 diff = receiverPos - startPos;
  float thisDistance = -(receiverFront * diff);

  float eqn2[4] = {receiverFront.x, receiverFront.y, receiverFront.z, thisDistance};
  glClipPlane(GL_CLIP_PLANE1, eqn2);
  glTranslatef(-receiverPos.x, -receiverPos.y, -receiverPos.z);

  glTranslatef(startPos.x, startPos.y, startPos.z);

  glEnable(GL_CLIP_PLANE0);
  glEnable(GL_CLIP_PLANE1);

  float texXInner = -g_gameTime / _radius;
  float texXOuter = -g_gameTime;

  //float texXInner = -1.0f/_radius;
  //float texXOuter = -1.0f;
  if (true)
  {
    glBegin(GL_QUAD_STRIP);

    for (int s = 0; s < numSteps; ++s)
    {
      LegacyVector3 deltaFrom = 1.2f * delta * static_cast<float>(s) / static_cast<float>(numSteps);
      LegacyVector3 deltaTo = 1.2f * delta * static_cast<float>(s + 1) / static_cast<float>(numSteps);

      LegacyVector3 currentPos = (-delta * 0.1f) + LegacyVector3(0, _radius, 0);

      for (int r = 0; r <= numRadii; ++r)
      {
        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, texXInner, r / numRadii);
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, texXOuter, r / numRadii);
        glVertex3fv((currentPos + deltaFrom).GetData());

        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, texXInner + 10.0f / static_cast<float>(numSteps), (r) / numRadii);
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, texXOuter + distance / (200.0f * static_cast<float>(numSteps)), (r) / numRadii);
        glVertex3fv((currentPos + deltaTo).GetData());

        currentPos.RotateAround(deltaNorm * (2.0f * M_PI / numRadii));
      }

      texXInner += 10.0f / static_cast<float>(numSteps);
      texXOuter += distance / (200.0f * static_cast<float>(numSteps));
    }

    glEnd();
  }
  glTranslatef(-startPos.x, -startPos.y, -startPos.z);

#ifdef USE_DIRECT3D
  SwapToModelMatrix();
#endif

  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
  glDepthMask(true);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);

  gglActiveTextureARB(GL_TEXTURE1_ARB);
  glDisable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  gglActiveTextureARB(GL_TEXTURE0_ARB);
  glDisable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  END_PROFILE(g_app->m_profiler, "Signal");
}

LegacyVector3 FeedingTube::GetStartPoint() { return GetDishPos(0.0f); }

LegacyVector3 FeedingTube::GetEndPoint() { return GetDishPos(0.0f) + (GetDishFront(0.0f) * m_range); }

bool FeedingTube::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  // This method is overridden for Radar Dish in order to expand the number
  // of cells the Radar Dish is placed into.  We were having problems where
  // entities couldn't get into the radar dish because its door was right on
  // the edge of an obstruction grid cell, so the entity didn't see the
  // building at all

  SpherePackage sphere(_pos, _radius * 1.5f);
  Matrix34 transform(m_front, m_up, m_pos);
  return m_shape->SphereHit(&sphere, transform);
}

int FeedingTube::GetBuildingLink() { return m_receiverId; }

void FeedingTube::SetBuildingLink(int _buildingId)
{
  Building* b = g_app->m_location->GetBuilding(_buildingId);
  if (b && b->m_buildingType == BuildingType::TypeFeedingTube)
  {
    m_receiverId = _buildingId;

    auto p = static_cast<FeedingTube*>(b);
  }
}

// *** Read
void FeedingTube::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_receiverId = atoi(_in->GetNextToken());
}

bool FeedingTube::IsInView()
{
  LegacyVector3 startPoint = GetStartPoint();
  LegacyVector3 endPoint = GetEndPoint();

  LegacyVector3 midPoint = (startPoint + endPoint) / 2.0f;
  float radius = (startPoint - endPoint).Mag() / 2.0f;
  radius += m_radius;

  if (g_app->m_camera->SphereInViewFrustum(midPoint, radius))
    return true;

  return Building::IsInView();
}
