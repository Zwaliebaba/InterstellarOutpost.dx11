#include "pch.h"
#include "RoutingSystem.h"
#include "LegacyVector2.h"
#include "GameApp.h"
#include "landscape.h"
#include "location.h"
#include "math_utils.h"
#include "radardish.h"

WayPoint::WayPoint(const int _type, const LegacyVector3& _pos)
  : m_pos(_pos),
    m_type(_type),
    m_buildingId(-1) {}

WayPoint::~WayPoint() {}

// *** GetPos
// I could have made it so that the Y values for GroundPoses were set once at
// level load time, but that would require an init phases after level file
// parsing and landscape terrain generation.
LegacyVector3 WayPoint::GetPos() const
{
  LegacyVector3 rv(m_pos);

  if (m_type == TypeGroundPos)
  {
    Landscape* land = &g_app->m_location->m_landscape;
    rv.y = land->m_heightMap->GetValue(rv.x, rv.z);
    if (rv.y < 0.0f)
      rv.y = 0.0f;
    rv.y += 1.0f;
  }
  else if (m_type == TypeBuilding)
  {
    Building* building = g_app->m_location->GetBuilding(m_buildingId);
    if (building)
    {
      DEBUG_ASSERT(building->m_buildingType == BuildingType::TypeRadarDish || building->m_buildingType == BuildingType::TypeBridge);
      auto teleport = static_cast<Teleport*>(building);
      LegacyVector3 pos, front;
      teleport->GetEntrance(pos, front);
      return pos;
    }
  }

  return rv;
}

void WayPoint::SetPos(const LegacyVector3& _pos) { m_pos = _pos; }

Route::Route(const int _id)
  : m_id(_id) {}

Route::~Route() { m_wayPoints.EmptyAndDelete(); }

void Route::AddWayPoint(const LegacyVector3& _pos)
{
  auto wayPoint = NEW WayPoint(WayPoint::Type3DPos, _pos);
  m_wayPoints.PutDataAtEnd(wayPoint);
}

void Route::AddWayPoint(const int _buildingId)
{
  auto wayPoint = NEW WayPoint(WayPoint::TypeBuilding, g_zeroVector);
  wayPoint->m_buildingId = _buildingId;
  m_wayPoints.PutDataAtEnd(wayPoint);
}

WayPoint* Route::GetWayPoint(const int _id)
{
  if (m_wayPoints.ValidIndex(_id))
  {
    WayPoint* wayPoint = m_wayPoints[_id];
    return wayPoint;
  }

  return nullptr;
}

int Route::GetIdOfNearestWayPoint(const LegacyVector3& _pos)
{
  int idOfNearest = -1;
  float distToNearestSqrd = FLT_MAX;

  int size = m_wayPoints.Size();
  for (int i = 0; i < size; ++i)
  {
    WayPoint* wp = m_wayPoints.GetData(i);
    LegacyVector3 delta = _pos - wp->GetPos();
    float distSqrd = delta.MagSquared();
    if (distSqrd < distToNearestSqrd)
    {
      distToNearestSqrd = distSqrd;
      idOfNearest = i;
    }
  }

  return idOfNearest;
}

// Returns the id of the first waypoint of the nearest edge
int Route::GetIdOfNearestEdge(const LegacyVector3& _pos, float* _dist)
{
  int idOfNearest = 0;
  float distToNearest = FLT_MAX;

  LegacyVector2 pos(_pos.x, _pos.z);
  WayPoint* wp = m_wayPoints.GetData(0);
  LegacyVector3 newPos = wp->GetPos();
  LegacyVector2 oldPos(newPos.x, newPos.z);

  int size = m_wayPoints.Size();
  for (int i = 1; i < size; ++i)
  {
    wp = m_wayPoints.GetData(i);
    newPos = wp->GetPos();
    LegacyVector2 temp(newPos.x, newPos.z);
    float dist = PointSegDist2D(pos, oldPos, temp);
    oldPos = temp;

    if (dist < distToNearest)
    {
      distToNearest = dist;
      idOfNearest = i;
    }
  }

  return idOfNearest - 1;
}
