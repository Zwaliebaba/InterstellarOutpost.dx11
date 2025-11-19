#ifndef INCLUDED_ROUTING_SYSTEM_H
#define INCLUDED_ROUTING_SYSTEM_H

#include "LegacyVector3.h"
#include "llist.h"

class WayPoint
{
  protected:
    LegacyVector3 m_pos;

  public:
    enum
    {
      Type3DPos,
      TypeGroundPos,
      TypeBuilding
    };

    int m_type;
    int m_buildingId;

    WayPoint(int _type, const LegacyVector3& _pos);
    ~WayPoint();

    LegacyVector3 GetPos() const;
    void SetPos(const LegacyVector3& _pos);
};

class Route
{
  public:
    int m_id;
    LList<WayPoint*> m_wayPoints;

    Route(int _id);
    ~Route();

    void AddWayPoint(const LegacyVector3& _pos);
    void AddWayPoint(int _buildingId);
    WayPoint* GetWayPoint(int _id);

    int GetIdOfNearestWayPoint(const LegacyVector3& _pos);
    int GetIdOfNearestEdge(const LegacyVector3& _pos, float* _dist);
};

#endif
