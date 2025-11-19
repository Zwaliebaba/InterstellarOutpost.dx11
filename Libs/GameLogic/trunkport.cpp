#include "pch.h"
#include "trunkport.h"
#include "DX9TextRenderer.h"
#include "GameApp.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "main.h"
#include "profiler.h"
#include "resource.h"
#include "shape.h"
#include "soundsystem.h"
#include "text_stream_readers.h"

TrunkPort::TrunkPort()
  : Building(BuildingType::TypeTrunkPort),
    m_targetLocationId(-1),
    m_heightMapSize(TRUNKPORT_HEIGHTMAP_MAXSIZE),
    m_openTimer(0.0f)
{
  Building::SetShape(Resource::GetShape("trunkport.shp"));

  m_destination1 = m_shape->m_rootFragment->LookupMarker("MarkerDestination1");
  m_destination2 = m_shape->m_rootFragment->LookupMarker("MarkerDestination2");

  m_heightMap.resize(m_heightMapSize * m_heightMapSize);
}

void TrunkPort::Initialize(Building* _template)
{
  Building::Initialize(_template);

  m_targetLocationId = dynamic_cast<TrunkPort*>(_template)->m_targetLocationId;

  ShapeMarker* marker = m_shape->m_rootFragment->LookupMarker("MarkerSurface");
  DEBUG_ASSERT(marker);

  Matrix34 transform(m_front, g_upVector, m_pos);
  LegacyVector3 worldPos = marker->GetWorldMatrix(transform).pos;

  float size = 90.0f;
  LegacyVector3 up = g_upVector * size;
  LegacyVector3 right = (m_front ^ g_upVector).Normalize() * size;

  for (int x = 0; x < m_heightMapSize; ++x)
  {
    for (int z = 0; z < m_heightMapSize; ++z)
    {
      float fractionX = static_cast<float>(x) / static_cast<float>(m_heightMapSize - 1);
      float fractionZ = static_cast<float>(z) / static_cast<float>(m_heightMapSize - 1);

      fractionX -= 0.5f;
      fractionZ -= 0.5f;

      LegacyVector3 basePos = worldPos;
      basePos += right * fractionX;
      basePos += up * fractionZ;

      m_heightMap[z * m_heightMapSize + x] = basePos;
    }
  }
}

bool TrunkPort::Advance()
{
  GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
  if (gb && gb->m_online && m_openTimer == 0.0f)
  {
    m_openTimer = GetHighResTime();
    g_app->m_soundSystem->TriggerBuildingEvent(this, "PowerUp");
  }

  return Building::Advance();
}

void TrunkPort::Render(float predictionTime)
{
  Building::Render(predictionTime);

  std::string caption;

  auto locationName = g_app->m_globalWorld->GetLocationNameTranslated(m_targetLocationId);
  if (!locationName.empty())
    caption = locationName;
  else
    caption = std::format("[%s]", Strings::Get("location_unknown"));

  START_PROFILE(g_app->m_profiler, "RenderDestination");

  float fontSize = 70.0f / static_cast<float>(caption.size());
  fontSize = std::min(fontSize, 10.0f);

  Matrix34 portMat(m_front, g_upVector, m_pos);

  Matrix34 destMat = m_destination1->GetWorldMatrix(portMat);
  glColor4f(0.9f, 0.8f, 0.8f, 1.0f);
  g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, caption);
  g_gameFont.SetRenderShadow(true);
  destMat.pos += destMat.f * 0.1f;
  destMat.pos += (destMat.f ^ destMat.u) * 0.2f;
  destMat.pos += destMat.u * 0.2f;
  glColor4f(0.9f, 0.8f, 0.8f, 0.0f);
  g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, caption);

  g_gameFont.SetRenderShadow(false);
  glColor4f(0.9f, 0.8f, 0.8f, 1.0f);
  destMat = m_destination2->GetWorldMatrix(portMat);
  g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, caption);
  g_gameFont.SetRenderShadow(true);
  destMat.pos += destMat.f * 0.1f;
  destMat.pos += (destMat.f ^ destMat.u) * 0.2f;
  destMat.pos += destMat.u * 0.2f;
  glColor4f(0.9f, 0.8f, 0.8f, 0.0f);
  g_gameFont.DrawText3D(destMat.pos, destMat.f, destMat.u, fontSize, caption);

  g_gameFont.SetRenderShadow(false);

  END_PROFILE(g_app->m_profiler, "RenderDestination");
}

bool TrunkPort::PerformDepthSort(LegacyVector3& _centerPos)
{
  _centerPos = m_centerPos;

  return true;
}

void TrunkPort::RenderAlphas(float predictionTime)
{
  Building::RenderAlphas(predictionTime);

  if (m_openTimer > 0.0f)
  {
    ShapeMarker* marker = m_shape->m_rootFragment->LookupMarker("MarkerSurface");
    DEBUG_ASSERT(marker);

    Matrix34 transform(m_front, g_upVector, m_pos);
    LegacyVector3 markerPos = marker->GetWorldMatrix(transform).pos;

    float timeOpen = GetHighResTime() - m_openTimer;
    float timeScale = std::max(5 - timeOpen, 1.0f);

    //
    // Calculate our Dif map based on some nice sine curves

    START_PROFILE(g_app->m_profiler, "Advance Heightmap");

    LegacyVector3 difMap[TRUNKPORT_HEIGHTMAP_MAXSIZE][TRUNKPORT_HEIGHTMAP_MAXSIZE];

    for (int x = 0; x < m_heightMapSize; ++x)
    {
      for (int z = 0; z < m_heightMapSize; ++z)
      {
        float maxDistance = 40.0f;
        float centerDif = (m_heightMap[z * m_heightMapSize + x] - markerPos).Mag();
        float fractionOut = centerDif / maxDistance;
        fractionOut = std::min(fractionOut, 1.0f);

        float wave1 = cos(centerDif * 0.15f);
        float wave2 = cos(centerDif * 0.05f);

        LegacyVector3 thisDif = m_front * sinf(g_gameTime * 2) * wave1 * (1.0f - fractionOut) * 15 * timeScale;
        thisDif += m_front * sin(g_gameTime * 2.5f) * wave2 * (1.0f - fractionOut) * 15 * timeScale;
        thisDif += g_upVector * cos(g_gameTime) * wave1 * timeScale * 10 * (1.0f - fractionOut);
        difMap[x][z] = thisDif;
      }
    }

    END_PROFILE(g_app->m_profiler, "Advance Heightmap");

    //
    // Render our height map with the dif map added on

    START_PROFILE(g_app->m_profiler, "Render Heightmap");

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(false);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laserfence.bmp"));

    float alphaValue = timeOpen;
    if (alphaValue > 0.7f)
      alphaValue = 0.7f;
    glColor4f(0.8f, 0.8f, 1.0f, alphaValue);

    for (int x = 0; x < m_heightMapSize - 1; ++x)
    {
      for (int z = 0; z < m_heightMapSize - 1; ++z)
      {
        LegacyVector3 thisPos1 = m_heightMap[z * m_heightMapSize + x] + difMap[x][z];
        LegacyVector3 thisPos2 = m_heightMap[z * m_heightMapSize + x + 1] + difMap[x + 1][z];
        LegacyVector3 thisPos3 = m_heightMap[(z + 1) * m_heightMapSize + x + 1] + difMap[x + 1][z + 1];
        LegacyVector3 thisPos4 = m_heightMap[(z + 1) * m_heightMapSize + x] + difMap[x][z + 1];

        float fractionX = static_cast<float>(x) / static_cast<float>(m_heightMapSize);
        float fractionZ = static_cast<float>(z) / static_cast<float>(m_heightMapSize);
        float width = 1.0f / m_heightMapSize;

        glBegin(GL_QUADS);
        glTexCoord2f(fractionX, fractionZ);
        glVertex3fv(thisPos1.GetData());
        glTexCoord2f(fractionX + width, fractionZ);
        glVertex3fv(thisPos2.GetData());
        glTexCoord2f(fractionX + width, fractionZ + width);
        glVertex3fv(thisPos3.GetData());
        glTexCoord2f(fractionX, fractionZ + width);
        glVertex3fv(thisPos4.GetData());
        glEnd();
      }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\glow.bmp"));
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    for (int x = 0; x < m_heightMapSize - 1; ++x)
    {
      for (int z = 0; z < m_heightMapSize - 1; ++z)
      {
        LegacyVector3 thisPos1 = m_heightMap[z * m_heightMapSize + x] + difMap[x][z];
        LegacyVector3 thisPos2 = m_heightMap[z * m_heightMapSize + x + 1] + difMap[x + 1][z];
        LegacyVector3 thisPos3 = m_heightMap[(z + 1) * m_heightMapSize + x + 1] + difMap[x + 1][z + 1];
        LegacyVector3 thisPos4 = m_heightMap[(z + 1) * m_heightMapSize + x] + difMap[x][z + 1];

        float fractionX = static_cast<float>(x) / static_cast<float>(m_heightMapSize);
        float fractionZ = static_cast<float>(z) / static_cast<float>(m_heightMapSize);
        float width = 1.0f / m_heightMapSize;

        glBegin(GL_QUADS);
        glTexCoord2f(fractionX, fractionZ);
        glVertex3fv(thisPos1.GetData());
        glTexCoord2f(fractionX + width, fractionZ);
        glVertex3fv(thisPos2.GetData());
        glTexCoord2f(fractionX + width, fractionZ + width);
        glVertex3fv(thisPos3.GetData());
        glTexCoord2f(fractionX, fractionZ + width);
        glVertex3fv(thisPos4.GetData());
        glEnd();
      }
    }

    glDisable(GL_TEXTURE_2D);

    glDepthMask(true);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    END_PROFILE(g_app->m_profiler, "Render Heightmap");
  }
}

void TrunkPort::ReprogramComplete()
{
  GlobalLocation* location = g_app->m_globalWorld->GetLocation(m_targetLocationId);
  if (location)
  {
    location->m_available = true;

    // Look for a "receiver" trunk port and set that to the same state
    for (int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i)
    {
      GlobalBuilding* building = g_app->m_globalWorld->m_buildings[i];
      if (building->m_buildingType == BuildingType::TypeTrunkPort && building->m_locationId == m_targetLocationId && building->
        m_link == g_app->m_locationId)
        building->m_online = true;
    }
  }

  Building::ReprogramComplete();
}

void TrunkPort::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  if (_in->TokenAvailable())
    m_targetLocationId = atoi(_in->GetNextToken());
}
