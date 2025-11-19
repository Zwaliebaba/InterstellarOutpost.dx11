#include "pch.h"
#include "researchitem.h"
#include "GameApp.h"
#include "camera.h"
#include "explosion.h"
#include "global_world.h"
#include "globals.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "preferences.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"
#include "taskmanager_interface.h"
#include "text_stream_readers.h"

ResearchItem::ResearchItem()
  : Building(BuildingType::TypeResearchItem),
    m_reprogrammed(100.0f),
    m_end1(nullptr),
    m_end2(nullptr),
    m_researchType(-1),
    m_level(1),
    m_inLibrary(false)
{
  m_researchType = GlobalResearch::TypeEngineer;

  Building::SetShape(Resource::GetShape("researchitem.shp"));

  m_front.RotateAroundY(frand(2.0f * M_PI));

  m_end1 = m_shape->m_rootFragment->LookupMarker("MarkerGrab1");
  m_end2 = m_shape->m_rootFragment->LookupMarker("MarkerGrab2");
}

void ResearchItem::Initialize(Building* _template)
{
  Building::Initialize(_template);

  m_researchType = dynamic_cast<ResearchItem*>(_template)->m_researchType;
  m_level = dynamic_cast<ResearchItem*>(_template)->m_level;

  m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
  m_pos.y += 20.0f;

  Matrix34 mat(m_front, m_up, m_pos);
  m_centerPos = m_shape->CalculateCenter(mat);
  m_radius = m_shape->CalculateRadius(mat, m_centerPos);
}

bool ResearchItem::Advance()
{
  if (m_vel.Mag() > 1.0f)
  {
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    m_vel *= (1.0f - SERVER_ADVANCE_PERIOD * 0.5f);

    Matrix34 mat(m_front, g_upVector, m_pos);
    m_centerPos = m_shape->CalculateCenter(mat);
  }
  else
    m_vel.Zero();

  if (m_researchType > -1 && g_app->m_globalWorld->m_research->HasResearch(m_researchType) && g_app->m_globalWorld->m_research->
    CurrentLevel(m_researchType) >= m_level)
    return true;

  if (m_reprogrammed <= 0.0f)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    g_explosionManager.AddExplosion(m_shape, mat, 1.0f);

    int existingLevel = g_app->m_globalWorld->m_research->CurrentLevel(m_researchType);

    g_app->m_globalWorld->m_research->AddResearch(m_researchType);
    g_app->m_globalWorld->m_research->m_researchLevel[m_researchType] = m_level;

    g_app->m_soundSystem->TriggerBuildingEvent(this, "AquireResearch");

    if (existingLevel == 0)
      g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageResearch, m_researchType, 4.0f);
    else
      g_app->m_taskManagerInterface->SetCurrentMessage(TaskManagerInterface::MessageResearchUpgrade, m_researchType, 4.0f);

    return true;
  }

  return false;
}

bool ResearchItem::NeedsReprogram() { return (m_reprogrammed > 0.0f); }

bool ResearchItem::Reprogram()
{
  m_reprogrammed -= SERVER_ADVANCE_PERIOD * 3.0f;

  return (m_reprogrammed <= 0.0f);
}

void ResearchItem::GetEndPositions(LegacyVector3& _end1, LegacyVector3& _end2)
{
  Matrix34 mat(m_front, m_up, m_pos);

  _end1 = m_end1->GetWorldMatrix(mat).pos;
  _end2 = m_end2->GetWorldMatrix(mat).pos;
}

void ResearchItem::Render(float _predictionTime)
{
  if (m_reprogrammed <= 0.0f)
    return;

  LegacyVector3 rotateAround = g_upVector;
  rotateAround.RotateAroundX(g_gameTime * 1.0f);
  rotateAround.RotateAroundZ(g_gameTime * 0.7f);
  rotateAround.Normalize();

  m_front.RotateAround(rotateAround * g_advanceTime);
  m_up.RotateAround(rotateAround * g_advanceTime);

  LegacyVector3 predictedPos = m_pos + m_vel * _predictionTime;
  Matrix34 mat(m_front, m_up, predictedPos);

  m_shape->Render(0.0f, mat);
}

void ResearchItem::RenderAlphas(float _predictionTime)
{
  Building::RenderAlphas(_predictionTime);

  LegacyVector3 camUp = g_app->m_camera->GetUp();
  LegacyVector3 camRight = g_app->m_camera->GetRight();

  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\cloudyglow.bmp"));

  float timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0f;

  int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
  int maxBlobs = 20;
  if (buildingDetail == 2)
    maxBlobs = 10;
  if (buildingDetail == 3)
    maxBlobs = 0;

  float alpha = 1.0f;

  for (int i = 0; i < maxBlobs; ++i)
  {
    LegacyVector3 pos = m_centerPos;
    pos.x += sinf(timeIndex + i) * i * 0.3f;
    pos.y += cosf(timeIndex + i) * sinf(i * 10) * 5;
    pos.z += cosf(timeIndex + i) * i * 0.3f;

    float size = 5.0f + sinf(timeIndex + i * 10) * 7.0f;
    size = std::max(size, 2.0f);

    //glColor4f( 0.6f, 0.2f, 0.1f, alpha);
    glColor4f(0.1f, 0.2f, 0.8f, alpha);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((pos - camRight * size + camUp * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((pos + camRight * size + camUp * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((pos + camRight * size - camUp * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((pos - camRight * size - camUp * size).GetData());
    glEnd();
  }

  //
  // Starbursts

  alpha = 1.0f - m_reprogrammed / 100.0f;
  alpha *= 0.3f;

  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));

  if (alpha > 0.0f)
  {
    int numStars = 10;
    if (buildingDetail == 2)
      numStars = 5;
    if (buildingDetail == 3)
      numStars = 2;

    for (int i = 0; i < numStars; ++i)
    {
      LegacyVector3 pos = m_centerPos;
      pos.x += sinf(timeIndex + i) * i * 0.3f;
      pos.y += (cosf(timeIndex + i) * cosf(i * 10) * 2);
      pos.z += cosf(timeIndex + i) * i * 0.3f;

      float size = i * 10 * alpha;
      if (i > numStars - 2)
        size = i * 20 * alpha;

      //glColor4f( 1.0f, 0.4f, 0.2f, alpha );
      glColor4f(0.1f, 0.2f, 0.8f, alpha);

      glBegin(GL_QUADS);
      glTexCoord2i(0, 0);
      glVertex3fv((pos - camRight * size + camUp * size).GetData());
      glTexCoord2i(1, 0);
      glVertex3fv((pos + camRight * size + camUp * size).GetData());
      glTexCoord2i(1, 1);
      glVertex3fv((pos + camRight * size - camUp * size).GetData());
      glTexCoord2i(0, 1);
      glVertex3fv((pos - camRight * size - camUp * size).GetData());
      glEnd();
    }
  }

  //
  // Draw control line to heaven

  alpha = 1.0f - m_reprogrammed / 100.0f;
  alpha *= (0.5f + fabs(cosf(g_gameTime)) * 0.5f);

  if (alpha > 0.0f)
  {
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laser.bmp"));
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    float w = 10.0f * alpha;

    glBegin(GL_QUADS);
    glColor4f(0.1f, 0.2f, 0.8f, alpha);
    glTexCoord2i(0, 0);
    glVertex3fv((m_pos + LegacyVector3(0, -50, 0) - camRight * w).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((m_pos + LegacyVector3(0, -50, 0) + camRight * w).GetData());

    glColor4f(0.1f, 0.2f, 0.8f, 0.0f);
    glTexCoord2i(1, 1);
    glVertex3fv((m_pos + LegacyVector3(0, 1000, 0) + camRight * w).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((m_pos + LegacyVector3(0, 1000, 0) - camRight * w).GetData());
    glEnd();

    w *= 0.3f;

    glBegin(GL_QUADS);
    glColor4f(0.1f, 0.2f, 0.8f, alpha);
    glTexCoord2i(0, 0);
    glVertex3fv((m_pos + LegacyVector3(0, -50, 0) - camRight * w).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((m_pos + LegacyVector3(0, -50, 0) + camRight * w).GetData());

    glColor4f(0.1f, 0.2f, 0.8f, 0.0f);
    glTexCoord2i(1, 1);
    glVertex3fv((m_pos + LegacyVector3(0, 1000, 0) + camRight * w).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((m_pos + LegacyVector3(0, 1000, 0) - camRight * w).GetData());
    glEnd();
  }

  glShadeModel(GL_FLAT);
  glDisable(GL_TEXTURE_2D);
  glDepthMask(true);
}

void ResearchItem::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_researchType = GlobalResearch::GetType(_in->GetNextToken());

  if (_in->TokenAvailable())
    m_level = atoi(_in->GetNextToken());
}

bool ResearchItem::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool ResearchItem::DoesShapeHit(Shape* _shape, Matrix34 _transform) { return false; }

bool ResearchItem::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                              LegacyVector3* norm) { return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen); }

bool ResearchItem::IsInView()
{
  if (Building::IsInView())
    return true;

  if (g_app->m_camera->PosInViewFrustum(m_pos + LegacyVector3(0, g_app->m_camera->GetPos().y, 0)))
    return true;

  return false;
}
