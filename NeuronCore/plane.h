#ifndef INCLUDED_PLANE_H
#define INCLUDED_PLANE_H

#include "LegacyVector3.h"

class Plane
{
  public:
    LegacyVector3 m_normal;
    float m_distFromOrigin;

    Plane(const LegacyVector3& a, const LegacyVector3& b, const LegacyVector3& c);
    Plane(const LegacyVector3& normal, const LegacyVector3& pointInPlane);
    Plane(const LegacyVector3& normal, float distFromOrigin);
    void GetCartesianDefinition(float* x, float* y, float* z, float* d) const;
};

#endif
