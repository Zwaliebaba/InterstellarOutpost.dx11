#include "pch.h"
#include "blueprintstore.h"
#include "GameApp.h"

#include "camera.h"
#include "entity_grid.h"
#include "global_world.h"
#include "location.h"
#include "main.h"
#include "resource.h"
#include "shape.h"
#include "team.h"
#include "text_stream_readers.h"

BlueprintBuilding::BlueprintBuilding(BuildingType _type)
  : Building(_type),
    m_buildingLink(-1),
    m_infected(0.0f),
    m_segment(0),
    m_marker(nullptr) { m_vel.Zero(); }

void BlueprintBuilding::Initialize(Building* _template)
{
  Building::Initialize(_template);

  m_marker = m_shape->m_rootFragment->LookupMarker("MarkerBlueprint");
  DEBUG_ASSERT(m_marker);

  auto blueprintBuilding = static_cast<BlueprintBuilding*>(_template);

  m_buildingLink = blueprintBuilding->m_buildingLink;

  if (m_id.GetTeamId() == 1)
    m_infected = 100.0f;
  else
    m_infected = 0.0f;
}

bool BlueprintBuilding::Advance()
{
  auto blueprintBuilding = static_cast<BlueprintBuilding*>(g_app->m_location->GetBuilding(m_buildingLink));
  if (blueprintBuilding)
  {
    if (m_infected > 80.0f)
      blueprintBuilding->SendBlueprint(m_segment, true);
    if (m_infected < 20.0f)
      blueprintBuilding->SendBlueprint(m_segment, false);
  }

  return Building::Advance();
}

Matrix34 BlueprintBuilding::GetMarker(float _predictionTime)
{
  LegacyVector3 pos = m_pos + m_vel * _predictionTime;
  Matrix34 mat(m_front, g_upVector, pos);

  if (m_marker)
  {
    Matrix34 markerMat = m_marker->GetWorldMatrix(mat);
    return markerMat;
  }
  return mat;
}

bool BlueprintBuilding::IsInView()
{
  Building* link = g_app->m_location->GetBuilding(m_buildingLink);

  if (link)
  {
    LegacyVector3 midPoint = (link->m_centerPos + m_centerPos) / 2.0f;
    float radius = (link->m_centerPos - m_centerPos).Mag();
    radius += m_radius;
    return (g_app->m_camera->SphereInViewFrustum(midPoint, radius));
  }
  return Building::IsInView();
}

void BlueprintBuilding::Render(float _predictionTime)
{
  LegacyVector3 pos = m_pos + m_vel * _predictionTime;
  Matrix34 mat(m_front, g_upVector, pos);
  m_shape->Render(_predictionTime, mat);
}

void BlueprintBuilding::RenderAlphas(float _predictionTime)
{
  Building::RenderAlphas(_predictionTime);

  auto link = static_cast<BlueprintBuilding*>(g_app->m_location->GetBuilding(m_buildingLink));
  if (link)
  {
    float infected = m_infected / 100.0f;
    float linkInfected = link->m_infected / 100.0f;
    float ourTime = g_gameTime + m_id.GetUniqueId() + m_id.GetIndex();
    if (fabs(infected - linkInfected) < 0.01f)
      glColor4f(infected, 0.7f - infected * 0.7f, 0.0f, 0.1f + fabs(sinf(ourTime)) * 0.2f);
    else
      glColor4f(infected, 0.7f - infected * 0.7f, 0.0f, 0.5f + fabs(sinf(ourTime)) * 0.5f);

    LegacyVector3 ourPos = GetMarker(_predictionTime).pos;
    LegacyVector3 theirPos = link->GetMarker(_predictionTime).pos;

    LegacyVector3 rightAngle = (g_app->m_camera->GetPos() - ourPos) ^ (theirPos - ourPos);
    rightAngle.SetLength(20.0f);

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laser.bmp"));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(false);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((ourPos - rightAngle).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((ourPos + rightAngle).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((theirPos + rightAngle).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((theirPos - rightAngle).GetData());
    glEnd();

    glDepthMask(true);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
  }

  //g_editorFont.DrawText3DCenter( m_pos+LegacyVector3(0,50,0), 10.0f, "%d Infected %2.2f", m_segment, m_infected );
}

void BlueprintBuilding::SendBlueprint(int _segment, bool _infected)
{
  m_segment = _segment;

  if (_infected)
    m_infected += SERVER_ADVANCE_PERIOD * 10.0f;
  else
    m_infected -= SERVER_ADVANCE_PERIOD * 10.0f;

  m_infected = std::max(m_infected, 0.0f);
  m_infected = std::min(m_infected, 100.0f);
}

void BlueprintBuilding::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_buildingLink = atoi(_in->GetNextToken());
}

int BlueprintBuilding::GetBuildingLink() { return m_buildingLink; }

void BlueprintBuilding::SetBuildingLink(int _buildingId) { m_buildingLink = _buildingId; }

// ============================================================================

BlueprintStore::BlueprintStore()
  : BlueprintBuilding(BuildingType::TypeBlueprintStore)
{
  Building::SetShape(Resource::GetShape("blueprintstore.shp"));
}

const char* BlueprintStore::GetObjectiveCounter()
{
  static char result[256];

  float totalInfection = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
    totalInfection += m_segments[i];

  sprintf(result, "%s %d%%", Strings::Get("objective_totalinfection").c_str(),
          static_cast<int>(100.0f * totalInfection / static_cast<float>(BLUEPRINTSTORE_NUMSEGMENTS * 100.0f)));

  return result;
}

void BlueprintStore::Initialize(Building* _template)
{
  BlueprintBuilding::Initialize(_template);

  if (m_id.GetTeamId() == 1)
  {
    for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
      m_segments[i] = 100.0f;
  }
  else
  {
    for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
      m_segments[i] = 0.0f;
  }
}

void BlueprintStore::SendBlueprint(int _segment, bool _infected)
{
  float oldValue = m_segments[_segment];

  if (_infected)
    oldValue += SERVER_ADVANCE_PERIOD * 1.0f;
  else
    oldValue -= SERVER_ADVANCE_PERIOD * 1.0f;

  oldValue = std::max(oldValue, 0.0f);
  oldValue = std::min(oldValue, 100.0f);

  m_segments[_segment] = oldValue;
}

bool BlueprintStore::Advance()
{
  int fullyInfected = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (m_segments[i] == 100.0f)
      fullyInfected++;
  }
  int clean = BLUEPRINTSTORE_NUMSEGMENTS - fullyInfected;

  //
  // Spread our existing infection

  if (clean > 0)
  {
    float infectionChange = (fullyInfected * 0.9f) / static_cast<float>(clean);

    for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
    {
      if (m_segments[i] < 100.0f)
      {
        float oldValue = m_segments[i];
        oldValue += SERVER_ADVANCE_PERIOD * infectionChange;
        oldValue = std::max(oldValue, 0.0f);
        oldValue = std::min(oldValue, 100.0f);
        m_segments[i] = oldValue;
      }
    }
  }

  //
  // Are we clean?

  bool totallyClean = true;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (m_segments[i] > 0.0f)
    {
      totallyClean = false;
      break;
    }
  }

  if (totallyClean)
  {
    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    if (gb)
      gb->m_online = true;
  }

  return BlueprintBuilding::Advance();
}

int BlueprintStore::GetNumInfected()
{
  int result = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (!(m_segments[i] < 100.0f))
      ++result;
  }
  return result;
}

int BlueprintStore::GetNumClean()
{
  int result = 0;
  for (int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i)
  {
    if (!(m_segments[i] > 0.0f))
      ++result;
  }
  return result;
}

void BlueprintStore::GetDisplay(LegacyVector3& _pos, LegacyVector3& _right, LegacyVector3& _up, float& _size)
{
  _size = 50.0f;

  LegacyVector3 front = m_front;
  front.RotateAroundY(sinf(g_gameTime) * 0.3f);

  LegacyVector3 up = g_upVector;
  up.RotateAround(m_front * cosf(g_gameTime) * 0.1f);

  _pos = m_pos + up * 50.0f - front * _size;
  _right = front;
  _up = up;
}

void BlueprintStore::Render(float _predictionTime) { BlueprintBuilding::Render(_predictionTime); }

void BlueprintStore::RenderAlphas(float _predictionTime)
{
  BlueprintBuilding::RenderAlphas(_predictionTime);

  LegacyVector3 screenPos, screenRight, screenUp;
  float screenSize;
  GetDisplay(screenPos, screenRight, screenUp, screenSize);

  //
  // Render main darwinian

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites\\darwinian.bmp"));
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glDepthMask(false);

  float texX = 0.0f;
  float texY = 0.0f;
  float texH = 1.0f;
  float texW = 1.0f;

  glShadeModel(GL_SMOOTH);

  glBegin(GL_QUADS);
  float infected = m_segments[0] / 100.0f;
  glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
  glTexCoord2f(texX, texY);
  glVertex3fv(screenPos.GetData());

  infected = m_segments[1] / 100.0f;
  glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
  glTexCoord2f(texX + texW, texY);
  glVertex3fv((screenPos + screenRight * screenSize * 2).GetData());

  infected = m_segments[2] / 100.0f;
  glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
  glTexCoord2f(texX + texW, texY + texH);
  glVertex3fv((screenPos + screenRight * screenSize * 2 + screenUp * screenSize * 2).GetData());

  infected = m_segments[3] / 100.0f;
  glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
  glTexCoord2f(texX, texY + texH);
  glVertex3fv((screenPos + screenUp * screenSize * 2).GetData());
  glEnd();

  //
  // Render lines for over effect

  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\interface_grey.bmp"));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  texX = 0.0f;
  texW = 3.0f;
  texY = g_gameTime * 0.01f;
  texH = 0.3f;

  for (int i = 0; i < 2; ++i)
  {
    glBegin(GL_QUADS);
    float infected = m_segments[0] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX, texY);
    glVertex3fv(screenPos.GetData());

    infected = m_segments[1] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX + texW, texY);
    glVertex3fv((screenPos + screenRight * screenSize * 2).GetData());

    infected = m_segments[2] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX + texW, texY + texH);
    glVertex3fv((screenPos + screenRight * screenSize * 2 + screenUp * screenSize * 2).GetData());

    infected = m_segments[3] / 100.0f;
    glColor4f(infected * 0.8f, 0.8f - infected * 0.8f, 0.0f, 1.0f);
    glTexCoord2f(texX, texY + texH);
    glVertex3fv((screenPos + screenUp * screenSize * 2).GetData());
    glEnd();

    texY *= 1.5f;
    texH = 0.1f;
  }

  glShadeModel(GL_FLAT);

  glDepthMask(true);
  glDisable(GL_TEXTURE_2D);
}

// ============================================================================

BlueprintConsole::BlueprintConsole()
  : BlueprintBuilding(BuildingType::TypeBlueprintConsole)
{
  Building::SetShape(Resource::GetShape("blueprintconsole.shp"));
}

void BlueprintConsole::Initialize(Building* _template)
{
  BlueprintBuilding::Initialize(_template);

  auto console = static_cast<BlueprintConsole*>(_template);
  m_segment = console->m_segment;
}

void BlueprintConsole::RecalculateOwnership()
{
  int teamCount[NUM_TEAMS];
  memset(teamCount, 0, NUM_TEAMS * sizeof(int));

  for (int i = 0; i < GetNumPorts(); ++i)
  {
    WorldObjectId id = GetPortOccupant(i);
    if (id.IsValid())
      teamCount[id.GetTeamId()]++;
  }

  int winningTeam = -1;
  for (int i = 0; i < NUM_TEAMS; ++i)
  {
    if (teamCount[i] > 2 && winningTeam == -1)
      winningTeam = i;
    else if (winningTeam != -1 && teamCount[i] > 2 && teamCount[i] > teamCount[winningTeam])
      winningTeam = i;
  }

  if (winningTeam == -1)
    SetTeamId(255);
  else
    SetTeamId(winningTeam);
}

void BlueprintConsole::Read(TextReader* _in, bool _dynamic)
{
  BlueprintBuilding::Read(_in, _dynamic);

  m_segment = atoi(_in->GetNextToken());
}

bool BlueprintConsole::Advance()
{
  RecalculateOwnership();

  bool infected = (m_id.GetTeamId() == 1);
  bool clean = (m_id.GetTeamId() == 0);

  if (infected)
    SendBlueprint(m_segment, true);
  if (clean)
    SendBlueprint(m_segment, false);

  return BlueprintBuilding::Advance();
}

void BlueprintConsole::Render(float _predictionTime)
{
  BlueprintBuilding::Render(_predictionTime);

  Matrix34 mat(m_front, g_upVector, m_pos);
  m_shape->Render(0.0f, mat);
}

void BlueprintConsole::RenderPorts()
{
  for (int i = 0; i < GetNumPorts(); ++i)
  {
    LegacyVector3 portPos;
    LegacyVector3 portFront;
    GetPortPosition(i, portPos, portFront);

    LegacyVector3 portUp = g_upVector;
    Matrix34 mat(portFront, portUp, portPos);

    //
    // Render the status light

    float size = 6.0f;
    LegacyVector3 camR = g_app->m_camera->GetRight() * size;
    LegacyVector3 camU = g_app->m_camera->GetUp() * size;

    LegacyVector3 statusPos = s_controlPadStatus->GetWorldMatrix(mat).pos;
    statusPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
    statusPos.y += 5.0f;

    WorldObjectId occupantId = GetPortOccupant(i);
    if (!occupantId.IsValid())
      glColor4ub(150, 150, 150, 255);
    else
    {
      RGBAColor teamColor = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
      glColor4ubv(teamColor.GetData());
    }

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((statusPos - camR - camU).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((statusPos + camR - camU).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((statusPos + camR + camU).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((statusPos - camR + camU).GetData());
    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
  }
}

// ============================================================================

BlueprintRelay::BlueprintRelay()
  : BlueprintBuilding(BuildingType::TypeBlueprintRelay),
    m_altitude(400.0f)
{
  Building::SetShape(Resource::GetShape("blueprintrelay.shp"));
}

void BlueprintRelay::Initialize(Building* _template)
{
  BlueprintBuilding::Initialize(_template);

  auto blueprintRelay = static_cast<BlueprintRelay*>(_template);
  m_altitude = blueprintRelay->m_altitude;

  m_pos.y = m_altitude;
  Matrix34 mat(m_front, m_up, m_pos);
  m_centerPos = m_shape->CalculateCenter(mat);
  m_radius = m_shape->CalculateRadius(mat, m_centerPos);
}

bool BlueprintRelay::Advance()
{
  float ourTime = g_gameTime + m_id.GetUniqueId() + m_id.GetIndex();

  LegacyVector3 oldPos = m_pos;

  m_pos.x += sinf(ourTime / 1.5f) * 1.0f;
  m_pos.y += sinf(ourTime / 1.3f) * 1.0f;
  m_pos.z += cosf(ourTime / 1.7f) * 1.0f;

  m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

  m_centerPos = m_pos;

  return BlueprintBuilding::Advance();
}

void BlueprintRelay::Read(TextReader* _in, bool _dynamic)
{
  BlueprintBuilding::Read(_in, _dynamic);

  m_altitude = atof(_in->GetNextToken());
}
