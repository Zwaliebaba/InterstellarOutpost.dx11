#include "pch.h"
#include "gamecursor.h"
#include "Canvas.h"
#include "GameApp.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "camera.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "insertion_squad.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "profiler.h"
#include "radardish.h"
#include "Renderer.h"
#include "resource.h"
#include "targetcursor.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"

GameCursor::GameCursor()
  : m_selectionArrowBoost(0.0f),
    m_highlightingSomething(false),
    m_validPlacementOpportunity(false),
    m_moveableEntitySelected(false)
{
  m_cursorStandard = NEW MouseCursor("icons\\mouse_main.bmp");
  m_cursorStandard->SetHotspot(0.055f, 0.070f);
  m_cursorStandard->SetSize(25.0f);

  m_cursorPlacement = NEW MouseCursor("icons\\mouse_placement.bmp");
  m_cursorPlacement->SetHotspot(0.5f, 0.5f);
  m_cursorPlacement->SetSize(40.0f);

  m_cursorDisabled = NEW MouseCursor("icons\\mouse_disabled.bmp");
  m_cursorDisabled->SetHotspot(0.5f, 0.5f);
  m_cursorDisabled->SetSize(60.0f);
  m_cursorDisabled->SetColor(RGBAColor(255, 0, 0, 255));

  m_cursorMoveHere = NEW MouseCursor("icons\\mouse_movehere.bmp");
  m_cursorMoveHere->SetHotspot(0.5f, 0.5f);
  m_cursorMoveHere->SetSize(30.0f);
  m_cursorMoveHere->SetAnimation(true);
  m_cursorMoveHere->SetColor(RGBAColor(255, 255, 150, 255));

  m_cursorHighlight = NEW MouseCursor("icons\\mouse_highlight.bmp");
  m_cursorHighlight->SetHotspot(0.5f, 0.5f);
  m_cursorHighlight->SetAnimation(true);

  m_cursorTurretTarget = NEW MouseCursor("icons\\mouse_turrettarget.bmp");
  m_cursorTurretTarget->SetHotspot(0.5f, 0.5f);
  m_cursorTurretTarget->SetColor(RGBAColor(255, 255, 255, 255));
  m_cursorTurretTarget->SetShadowed(false);

  m_cursorSelection = NEW MouseCursor("icons\\mouse_selection.bmp");
  m_cursorSelection->SetHotspot(0.5f, 0.5f);

  m_cursorMissile = nullptr;
}

GameCursor::~GameCursor()
{
  SAFE_DELETE(m_cursorStandard);
  SAFE_DELETE(m_cursorPlacement);
  SAFE_DELETE(m_cursorDisabled);
  SAFE_DELETE(m_cursorMoveHere);
  SAFE_DELETE(m_cursorHighlight);
  SAFE_DELETE(m_cursorTurretTarget);
  SAFE_DELETE(m_cursorSelection);
  SAFE_DELETE(m_cursorMissile);
}

fire_and_forget GameCursor::LoadAssetsAsync()
{
  StartLoading();
  co_await resume_background();
  m_selectionArrowTexture = TextureManager::LoadTexture(L"icons\\selectionarrow.bmp");
  FinishLoading();
}

bool GameCursor::GetSelectedObject(WorldObjectId& _id, LegacyVector3& _pos)
{
  Team* team = g_app->m_location->GetMyTeam();

  if (team)
  {
    Entity* selectedEnt = team->GetMyEntity();
    if (selectedEnt)
    {
      _pos = selectedEnt->m_pos + selectedEnt->m_centerPos + selectedEnt->m_vel * g_predictionTime;
      _id = selectedEnt->m_id;
      return true;
    }
    if (team->m_currentBuildingId != -1)
    {
      Building* building = g_app->m_location->GetBuilding(team->m_currentBuildingId);
      if (building)
      {
        _pos = building->m_centerPos;
        _id = building->m_id;
        return true;
      }
    }
    else
    {
      Unit* selected = team->GetMyUnit();
      if (selected)
      {
        _pos = selected->m_centerPos + selected->m_vel * g_predictionTime;
        _id.Set(selected->m_teamId, selected->m_unitId, -1, -1);

        // Add the center pos
        for (int i = 0; i < selected->m_entities.Size(); ++i)
        {
          if (selected->m_entities.ValidIndex(i))
          {
            Entity* ent = selected->m_entities[i];
            _pos += ent->m_centerPos;
            break;
          }
        }

        return true;
      }
    }
  }

  return false;
}

bool GameCursor::GetHighlightedObject(WorldObjectId& _id, LegacyVector3& _pos, float& _radius)
{
  WorldObjectId id;
  bool somethingHighlighted = false;
  bool found = false;

  if (!g_app->m_taskManagerInterface->m_visible)
    somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse(id, g_app->m_globalWorld->m_myTeamId);
  else
  {
    Task* task = g_app->m_taskManager->GetTask(g_app->m_taskManagerInterface->m_highlightedTaskId);
    if (task && task->m_objId.IsValid())
    {
      id = task->m_objId;
      somethingHighlighted = true;
    }
  }

  if (somethingHighlighted)
  {
    if (id.GetUnitId() == UNIT_BUILDINGS)
    {
      // Found a building
      Building* building = g_app->m_location->GetBuilding(id.GetUniqueId());
      if (building->m_buildingType == BuildingType::TypeRadarDish || building->m_buildingType == BuildingType::TypeBridge || building->m_buildingType ==
        BuildingType::TypeGunTurret || building->m_buildingType == BuildingType::TypeFenceSwitch)
      {
        _id = id;
        _pos = building->m_centerPos;
        _radius = building->m_radius;
        found = true;
        if (building->m_buildingType == BuildingType::TypeGunTurret)
          _pos.y += _radius * 0.5f;
      }
    }
    else if (id.GetIndex() == -1)
    {
      // Found a unit
      Unit* unit = g_app->m_location->GetUnit(id);
      if (unit)
      {
        _id = id;
        _pos = unit->m_centerPos + unit->m_vel * g_predictionTime;
        _radius = unit->m_radius;
        found = true;

        // Add the center pos
        for (int i = 0; i < unit->m_entities.Size(); ++i)
        {
          if (unit->m_entities.ValidIndex(i))
          {
            Entity* ent = unit->m_entities[i];
            _pos += ent->m_centerPos;
            break;
          }
        }
      }
    }
    else
    {
      // Found an entity
      Entity* entity = g_app->m_location->GetEntity(id);
      _id = id;
      _pos = entity->m_pos + entity->m_vel * g_predictionTime + entity->m_centerPos;
      _radius = entity->m_radius * 1.5f;
      if (entity->m_entityType == EntityType::TypeDarwinian)
        _radius = entity->m_radius * 2.0f;
      found = true;
    }
  }

  return found;
}

void GameCursor::CreateMarker(const LegacyVector3& _pos)
{
  LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(_pos.x, _pos.z);
  LegacyVector3 front = (landNormal ^ g_upVector).Normalize();

  auto marker = NEW MouseCursorMarker();
  marker->m_pos = _pos;
  marker->m_front = front;
  marker->m_up = landNormal;
  marker->m_startTime = GetHighResTime();

  m_markers.PutData(marker);
}

void GameCursor::BoostSelectionArrows(float _seconds) { m_selectionArrowBoost = std::max(m_selectionArrowBoost, _seconds); }

void GameCursor::RenderMarkers()
{
  for (int i = 0; i < m_markers.Size(); ++i)
  {
    MouseCursorMarker* marker = m_markers[i];
    float timeSync = GetHighResTime() - marker->m_startTime;
    if (timeSync > 0.5f)
    {
      m_markers.RemoveData(i);
      delete marker;
      --i;
    }
    else
    {
      m_cursorPlacement->SetSize(20.0f - timeSync * 40.0f);
      m_cursorPlacement->SetAnimation(false);
      m_cursorPlacement->Render3D(marker->m_pos, marker->m_front, marker->m_up, false);
    }
  }
}

void GameCursor::Render()
{
  START_PROFILE(g_app->m_profiler, "Render GameCursor");

  float nearPlaneStart = g_app->m_renderer->GetNearPlane();
  g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05f, g_app->m_renderer->GetFarPlane());

  int screenX = g_target->X();
  int screenY = g_target->Y();
  LegacyVector3 mousePos = g_app->m_userInput->GetMousePos3d();
  mousePos.y = std::max(1.0f, mousePos.y);

  bool cursorRendered = false;

  m_highlightingSomething = false;
  m_validPlacementOpportunity = false;
  m_moveableEntitySelected = false;

  // Set mip mapping for game cursor
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (Canvas::EclGetWindows()->Size() <= 0)
  {
    if (!g_app->m_camera->IsInteractive())
    {
      // Cut scene of some sort, no player control, so no cursor
      cursorRendered = true;
    }
    else if (!g_app->m_location)
    {
      // We are in the global world
      GlobalLocation* highlightedLocation = g_app->m_globalWorld->GetHighlightedLocation();
      bool locAvailable = highlightedLocation && highlightedLocation->m_missionFilename != "null" && highlightedLocation->
        m_available;
      g_app->m_renderer->SetupMatricesFor2D();
      m_cursorPlacement->SetAnimation(locAvailable);
      m_cursorPlacement->SetSize(40.0f);
      m_cursorPlacement->Render(screenX, screenY);
      if (!locAvailable)
        m_cursorDisabled->Render(screenX, screenY);
      g_app->m_renderer->SetupMatricesFor3D();
      cursorRendered = true;
    }
    else if (g_app->m_location)
    {
      // We are at a location
      Task* task = g_app->m_taskManager->GetCurrentTask();

      WorldObjectId selectedId;
      LegacyVector3 selectedWorldPos;
      LegacyVector3 highlightedWorldPos;
      WorldObjectId highlightedId;
      float highlightedRadius;

      bool somethingSelected = GetSelectedObject(selectedId, selectedWorldPos);
      bool somethingHighlighted = GetHighlightedObject(highlightedId, highlightedWorldPos, highlightedRadius);

      if (g_app->m_taskManagerInterface->m_visible)
      {
        // Looking at the task manager
        if (somethingSelected && selectedId.GetUnitId() != UNIT_BUILDINGS)
          RenderSelectionArrows(selectedId, selectedWorldPos);

        if (somethingHighlighted)
        {
          float camDist = (g_app->m_camera->GetPos() - highlightedWorldPos).Mag();
          float posX, posY;
          g_app->m_camera->Get2DScreenPos(highlightedWorldPos, &posX, &posY);
          m_cursorSelection->SetSize(highlightedRadius * 100 / sqrt(camDist));
          m_cursorSelection->SetColor(RGBAColor(255, 255, 100, 255));
          m_cursorSelection->SetAnimation(false);
          g_app->m_renderer->SetupMatricesFor2D();
          m_cursorSelection->Render(posX, ClientEngine::OutputSize().Height - posY);
          g_app->m_renderer->SetupMatricesFor3D();
        }
      }
      else if (task && task->m_state == Task::StateStarted && task->m_type != GlobalResearch::TypeOfficer && !somethingHighlighted)
      {
        // The player is placing a task
        bool validPlacement = g_app->m_taskManager->IsValidTargetArea(task->m_id, mousePos);
        m_cursorPlacement->SetAnimation(validPlacement);
        m_cursorPlacement->SetSize(40.0f);
        LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
        LegacyVector3 front = (landNormal ^ g_upVector).Normalize();
        m_cursorPlacement->Render3D(mousePos, front, landNormal);
        if (!validPlacement)
          m_cursorDisabled->Render3D(mousePos, front, landNormal);
        else
          m_validPlacementOpportunity = true;

        cursorRendered = true;
      }
      else if (g_app->m_camera->IsInMode(Camera::ModeEntityTrack)) { cursorRendered = true; }
      else
      {
        if (somethingHighlighted && !(somethingSelected && highlightedId.GetUnitId() == UNIT_BUILDINGS))
        {
          float camDist = (g_app->m_camera->GetPos() - highlightedWorldPos).Mag();
          if (camDist > 100 || !somethingSelected || selectedId != highlightedId)
          {
            float posX, posY;
            g_app->m_camera->Get2DScreenPos(highlightedWorldPos, &posX, &posY);
            m_cursorSelection->SetSize(highlightedRadius * 100 / sqrt(camDist));
            m_cursorSelection->SetColor(RGBAColor(255, 255, 100, 255));
            m_cursorSelection->SetAnimation(false);
            g_app->m_renderer->SetupMatricesFor2D();
            m_cursorSelection->Render(posX, ClientEngine::OutputSize().Height - posY);
            g_app->m_renderer->SetupMatricesFor3D();

            m_highlightingSomething = true;
          }
        }

        if (somethingSelected && selectedId.GetUnitId() != UNIT_BUILDINGS)
        {
          EntityType entityType = EntityType::Invalid;
          if (selectedId.GetIndex() == -1)
            entityType = g_app->m_location->GetUnit(selectedId)->m_troopType;
          else
            entityType = g_app->m_location->GetEntity(selectedId)->m_entityType;

          RenderSelectionArrows(selectedId, selectedWorldPos);
          m_moveableEntitySelected = true;

          bool highlightedBuilding = (somethingHighlighted && highlightedId.GetUnitId() == UNIT_BUILDINGS);

          if ((entityType == EntityType::TypeSquadie || entityType == EntityType::TypeOfficer) && highlightedBuilding)
          {
            Building* building = g_app->m_location->GetBuilding(highlightedId.GetUniqueId());

            if (building && building->m_buildingType == BuildingType::TypeRadarDish)
            {
              // Squadies/officer trying to get into a teleport
              auto dish = dynamic_cast<RadarDish*>(building);
              if (dish->Connected())
              {
                LegacyVector3 entrancePos, entranceFront;
                dish->GetEntrance(entrancePos, entranceFront);
                float posX, posY;
                g_app->m_camera->Get2DScreenPos(entrancePos, &posX, &posY);
                g_app->m_renderer->SetupMatricesFor2D();
                m_cursorPlacement->SetSize(60.0f);
                m_cursorPlacement->SetAnimation(true);
                m_cursorPlacement->Render(posX, ClientEngine::OutputSize().Height - posY);
                g_app->m_renderer->SetupMatricesFor3D();
              }
            }
          }

          // Selected a unit OR an entity
          if (!somethingHighlighted || highlightedId.GetUnitId() == UNIT_BUILDINGS)
          {
            LegacyVector3 targetFront = (mousePos - selectedWorldPos).Normalize();
            LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
            LegacyVector3 targetRight = (targetFront ^ landNormal).Normalize();
            targetFront = (targetRight ^ landNormal).Normalize();
            m_cursorMoveHere->SetSize(30.0f);
            m_cursorMoveHere->Render3D(mousePos, targetFront, landNormal);
            cursorRendered = true;
          }
        }

        if (somethingSelected && selectedId.GetUnitId() == UNIT_BUILDINGS)
        {
          // Selected a building - render a targetting crosshair
          g_app->m_renderer->SetupMatricesFor2D();
          m_cursorTurretTarget->SetSize(200.0f);
          m_cursorTurretTarget->Render(screenX, screenY);
          g_app->m_renderer->SetupMatricesFor3D();
        }

        if (!somethingSelected && !somethingHighlighted)
        {
          // Looking at empty landscape
          LegacyVector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
          LegacyVector3 front = (landNormal ^ g_upVector).Normalize();
          m_cursorHighlight->SetAnimation(false);
          m_cursorHighlight->SetSize(30.0f);
          m_cursorHighlight->Render3D(mousePos, front, landNormal);
          cursorRendered = true;
        }
      }
    }
  }

  if (!cursorRendered && g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD)
  {
    // Nobody has drawn a cursor yet
    // So give us the default
    g_app->m_renderer->SetupMatricesFor2D();
    RenderStandardCursor(screenX, screenY);
    g_app->m_renderer->SetupMatricesFor3D();
  }

  RenderMarkers();

  g_app->m_camera->SetupProjectionMatrix(nearPlaneStart, g_app->m_renderer->GetFarPlane());

  END_PROFILE(g_app->m_profiler, "Render GameCursor");
}

void GameCursor::RenderStandardCursor(float _screenX, float _screenY) { m_cursorStandard->Render(_screenX, _screenY); }

void GameCursor::FindScreenEdge(const LegacyVector2& _line, float* _posX, float* _posY)
{
  //    y = mx + c
  //    c = y - mx
  //    x = (y - c) / m

  int screenH = ClientEngine::OutputSize().Height;
  int screenW = ClientEngine::OutputSize().Width;

  float m = _line.y / _line.x;
  float c = (screenH / 2.0f) - m * (screenW / 2.0f);

  if (_line.y < 0)
  {
    // Intersect with top view plane
    float x = (0 - c) / m;
    if (x >= 0 && x <= screenW)
    {
      *_posX = x;
      *_posY = 0;
      return;
    }
  }
  else
  {
    // Intersect with the bottom view plane
    float x = (screenH - c) / m;
    if (x >= 0 && x <= screenW)
    {
      *_posX = x;
      *_posY = screenH;
      return;
    }
  }

  if (_line.x < 0)
  {
    // Intersect with left view plane
    float y = m * 0 + c;
    if (y >= 0 && y <= screenH)
    {
      *_posX = 0;
      *_posY = y;
      return;
    }
  }
  else
  {
    // Intersect with right view plane
    float y = m * screenW + c;
    if (y >= 0 && y <= screenH)
    {
      *_posX = screenW;
      *_posY = y;
      return;
    }
  }

  // We should never ever get here
  DEBUG_ASSERT(false);
}

void GameCursor::RenderSelectionArrow(float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha)
{
  LegacyVector2 pos(_screenX, _screenY);
  LegacyVector2 gradient(_screenDX, _screenDY);
  LegacyVector2 rightAngle = gradient;
  float tempX = rightAngle.x;
  rightAngle.x = rightAngle.y;
  rightAngle.y = tempX * -1;

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDepthMask(false);

  glColor4f(1.0f, 1.0f, 0.3f, _alpha);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  auto text = m_selectionArrowTexture.lock();
  DEBUG_ASSERT(text != nullptr);
  glBindTexture(GL_TEXTURE_2D, text->m_ref);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2fv((pos - rightAngle * _size / 2.0f).GetData());
  glTexCoord2i(0, 0);
  glVertex2fv((pos - rightAngle * _size / 2.0f + gradient * _size).GetData());
  glTexCoord2i(1, 0);
  glVertex2fv((pos + rightAngle * _size / 2.0f + gradient * _size).GetData());
  glTexCoord2i(1, 1);
  glVertex2fv((pos + rightAngle * _size / 2.0f).GetData());
  glEnd();

  glDepthMask(true);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}

void GameCursor::RenderSelectionArrows(WorldObjectId _id, const LegacyVector3& _pos)
{
  Entity* ent = g_app->m_location->GetEntity(_id);
  Unit* unit = g_app->m_location->GetUnit(_id);
  if (!ent && !unit)
    return;
  if (ent && ent->m_dead)
    return;
  if (unit && unit->NumAliveEntities() == 0)
    return;

  float triSize = 40.0f;

  static bool onScreen = true;

  int screenH = ClientEngine::OutputSize().Height;
  int screenW = ClientEngine::OutputSize().Width;

  //
  // Project worldTarget into screen co-ordinates
  // Is the _pos on screen or not?

  float screenX, screenY;
  g_app->m_camera->Get2DScreenPos(_pos, &screenX, &screenY);
  screenY = screenH - screenY;

  LegacyVector3 toCam = g_app->m_camera->GetPos() - _pos;
  float angle = toCam * g_app->m_camera->GetFront();

  if (angle <= 0.0f && screenX >= 0 && screenX < screenW && screenY >= 0 && screenY < screenH)
  {
    // _pos is onscreen
    float camDist = toCam.Mag();
    m_selectionArrowBoost -= g_advanceTime * 0.4f;

    float distanceOut = 1000 / sqrtf(camDist);
    float alpha = std::min(m_selectionArrowBoost, 0.9f);

    if (camDist > 200.0f)
      alpha = std::max(std::min((camDist - 200.0f) / 200.0f, 0.9f), alpha);
    g_app->m_renderer->SetupMatricesFor2D();
    RenderSelectionArrow(screenX, screenY - distanceOut, 0, -1, triSize, alpha);
    RenderSelectionArrow(screenX, screenY + distanceOut, 0, 1, triSize, alpha);
    RenderSelectionArrow(screenX - distanceOut, screenY, -1, 0, triSize, alpha);
    RenderSelectionArrow(screenX + distanceOut, screenY, 1, 0, triSize, alpha);
    g_app->m_renderer->SetupMatricesFor3D();

    if (!onScreen)
      BoostSelectionArrows(2.0f);
    onScreen = true;
  }
  else
  {
    // _pos is offscreen
    LegacyVector3 camPos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * 1000;
    LegacyVector3 camToTarget = (_pos - camPos).SetLength(100);

    float camX = screenW / 2.0f;
    float camY = screenH / 2.0f;
    float posX, posY;
    g_app->m_camera->Get2DScreenPos(camPos + camToTarget, &posX, &posY);

    LegacyVector2 lineNormal(posX - camX, posY - camY);
    lineNormal.Normalize();

    float edgeX, edgeY;
    FindScreenEdge(lineNormal, &edgeX, &edgeY);

    lineNormal.x *= -1;

    g_app->m_renderer->SetupMatricesFor2D();
    RenderSelectionArrow(edgeX, screenH - edgeY, lineNormal.x, lineNormal.y, triSize * 1.5f, 0.9f);
    g_app->m_renderer->SetupMatricesFor3D();

    onScreen = false;
  }
}

void GameCursor::RenderWeaponMarker(LegacyVector3 _pos, LegacyVector3 _front, LegacyVector3 _up)
{
  m_cursorPlacement->SetSize(40.0f);
  m_cursorPlacement->SetShadowed(true);
  m_cursorPlacement->SetAnimation(true);
  m_cursorPlacement->Render3D(_pos, _front, _up);
}

MouseCursor::MouseCursor(const char* _filename)
  : m_size(20.0f),
    m_animating(false),
    m_shadowed(true),
    m_hotspotX(0.0f),
    m_hotspotY(0.0f)
{
  m_mainFilename = _filename;

  BinaryReader* binReader = Resource::GetBinaryReader(m_mainFilename.c_str());
  ASSERT_TEXT(binReader, "Failed to open mouse cursor resource %s", _filename);
  BitmapRGBA bmp(binReader, "bmp");
  SAFE_DELETE(binReader);

  Resource::AddBitmap(m_mainFilename.c_str(), bmp);

  m_shadowFilename = "shadow_" + m_mainFilename;
  bmp.ApplyBlurFilter(10.0f);
  Resource::AddBitmap(m_shadowFilename.c_str(), bmp);

  m_colour.Set(255, 255, 255, 255);
}

MouseCursor::~MouseCursor()
{
}

float MouseCursor::GetSize()
{
  float size = m_size;

  if (m_animating)
    size += fabs(sinf(g_gameTime * 4.0f)) * size * 0.6f;

  return size;
}

void MouseCursor::SetSize(float _size) { m_size = _size; }

void MouseCursor::SetAnimation(bool _onOrOff) { m_animating = _onOrOff; }

void MouseCursor::SetShadowed(bool _onOrOff) { m_shadowed = _onOrOff; }

void MouseCursor::SetHotspot(float x, float y)
{
  m_hotspotX = x;
  m_hotspotY = y;
}

void MouseCursor::SetColor(const RGBAColor& _col) { m_colour = _col; }

void MouseCursor::Render(float _x, float _y)
{
  float s = GetSize();
  float x = _x - s * m_hotspotX;
  float y = _y - s * m_hotspotY;

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDepthMask(false);

  if (m_shadowed)
  {
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(m_shadowFilename.c_str()));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex2f(x, y);
    glTexCoord2i(1, 1);
    glVertex2f(x + s, y);
    glTexCoord2i(1, 0);
    glVertex2f(x + s, y + s);
    glTexCoord2i(0, 0);
    glVertex2f(x, y + s);
    glEnd();
  }

  glColor4ubv(m_colour.GetData());
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(m_mainFilename.c_str()));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2f(x, y);
  glTexCoord2i(1, 1);
  glVertex2f(x + s, y);
  glTexCoord2i(1, 0);
  glVertex2f(x + s, y + s);
  glTexCoord2i(0, 0);
  glVertex2f(x, y + s);
  glEnd();

  glDepthMask(true);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}

void MouseCursor::Render3D(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up, bool _cameraScale)
{
  LegacyVector3 rightAngle = (_front ^ _up).Normalize();

  glEnable(GL_TEXTURE_2D);

  glEnable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  //glDisable       (GL_DEPTH_TEST );
  glDepthMask(false);

  float scale = GetSize();
  if (_cameraScale)
  {
    float camDist = (g_app->m_camera->GetPos() - _pos).Mag();
    scale *= sqrtf(camDist) / 40.0f;
  }

  LegacyVector3 pos = _pos;
  pos -= m_hotspotX * rightAngle * scale;
  pos += m_hotspotY * _front * scale;

  if (m_shadowed)
  {
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(m_shadowFilename.c_str()));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glColor4f(1.0f, 1.0f, 1.0f, 0.0f);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex3fv((pos).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((pos + rightAngle * scale).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((pos - _front * scale + rightAngle * scale).GetData());
    glTexCoord2i(0, 0);
    glVertex3fv((pos - _front * scale).GetData());
    glEnd();
  }

  glColor4ubv(m_colour.GetData());
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(m_mainFilename.c_str()));
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex3fv((pos).GetData());
  glTexCoord2i(1, 1);
  glVertex3fv((pos + rightAngle * scale).GetData());
  glTexCoord2i(1, 0);
  glVertex3fv((pos - _front * scale + rightAngle * scale).GetData());
  glTexCoord2i(0, 0);
  glVertex3fv((pos - _front * scale).GetData());
  glEnd();

  glDepthMask(true);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
}
