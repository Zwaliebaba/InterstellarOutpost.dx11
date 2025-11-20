#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <stdlib.h>

#include "random.h"

class LegacyVector3;
class LegacyVector2;
class Matrix34;
class Plane;

inline float frand(float range = 1.0f)
{
  return range * (static_cast<float>(darwiniaRandom()) / static_cast<float>(DARWINIA_RAND_MAX));
}

inline float sfrand(float range = 1.0f)
{
  return (0.5f - static_cast<float>(darwiniaRandom()) / static_cast<float>((DARWINIA_RAND_MAX))) * range;
}

// Network Syncronised versions (only call inside Net-Safe code)
unsigned long syncrand();
inline float syncfrand(float range = 1.0f) { return syncrand() * (range / 4294967296.0f); }
inline float syncsfrand(float range = 1.0f) { return (syncfrand() - 0.5f) * range; }

#ifndef M_PI
#define M_PI 3.1415926535897932384626f
#endif

#define sign(a)				((a) < 0 ? -1 : 1)
#define signf(a)			((a) < 0.0f ? -1.0f : 1.0f)

#define Round(a) (int)(a)

#define Clamp(a, low, high)	if (a>high) a = high; else if (a<low) a = low;

#define NearlyEquals(a, b)	(fabsf((a) - (b)) < 1e-6 ? 1 : 0)

#define ASSERT_FLOAT_IS_SANE(x) \
	DEBUG_ASSERT((x + 1.0f) != x);
#define ASSERT_VECTOR3_IS_SANE(v) \
	ASSERT_FLOAT_IS_SANE((v).x) \
	ASSERT_FLOAT_IS_SANE((v).y) \
	ASSERT_FLOAT_IS_SANE((v).z)

// Imagine that you have a stationary object that you want to move from
// A to B in _duration. This function helps you do that with linear acceleration
// to the midpoint, then linear deceleration to the end point. It returns the
// fractional distance along the route. Used in the camera MoveToTarget routine.
double RampUpAndDown(double _startTime, double _duration, double _timeNow);

// **********************
// General Geometry Utils
// **********************

void GetPlaneMatrix(const LegacyVector3& t1, const LegacyVector3& t2, const LegacyVector3& t3, Matrix34* mat);
float ProjectPointOntoPlane(const LegacyVector3& point, const Matrix34& planeMat, LegacyVector3* result);
void ConvertWorldSpaceIntoPlaneSpace(const LegacyVector3& point, const Matrix34& plane, LegacyVector2* result);
void ConvertPlaneSpaceIntoWorldSpace(const LegacyVector2& point, const Matrix34& plane, LegacyVector3* result);
float CalcTriArea(const LegacyVector2& t1, const LegacyVector2& t2, const LegacyVector2& t3);

// *********************
// 2D Intersection Tests
// *********************

bool IsPointInTriangle(const LegacyVector2& pos, const LegacyVector2& t1, const LegacyVector2& t2, const LegacyVector2& t3);
float PointSegDist2D(const LegacyVector2& p,	// Point
                     const LegacyVector2& l0, const LegacyVector2& l1, // Line seg
                     LegacyVector2* result = nullptr);
bool SegRayIntersection2D(const LegacyVector2& _lineStart, const LegacyVector2& _lineEnd, const LegacyVector2& _rayStart,
                          const LegacyVector2& _rayDir, LegacyVector2* _result = nullptr);

// *********************
// 3D Intersection Tests
// *********************

float RayRayDist(const LegacyVector3& a, const LegacyVector3& aDir, const LegacyVector3& b, const LegacyVector3& bDir,
                 LegacyVector3* posOnA = nullptr, LegacyVector3* posOnB = nullptr);

float RaySegDist(const LegacyVector3& pointOnLine, const LegacyVector3& lineDir, const LegacyVector3& segStart,
                 const LegacyVector3& segEnd, LegacyVector3* posOnRay = nullptr, LegacyVector3* posInSeg = nullptr);

bool RayTriIntersection(const LegacyVector3& orig, const LegacyVector3& dir, const LegacyVector3& vert0, const LegacyVector3& vert1,
                        const LegacyVector3& vert2, float _rayLen = 1e10, LegacyVector3* result = nullptr);

int RayPlaneIntersection(const LegacyVector3& pOnLine, const LegacyVector3& lineDir, const Plane& plane,
                         LegacyVector3* intersectionPoint = nullptr);
//bool RayPlaneIntersection       (LegacyVector3 const &rayStart, LegacyVector3 const &rayDir,
//                                  LegacyVector3 const &planePos, LegacyVector3 const &planeNormal,
//						         float _rayLen=1e10, LegacyVector3 *pos=NULL );

bool XM_CALLCONV RaySphereIntersection(XMVECTOR rayStart, XMVECTOR rayDir, XMVECTOR spherePos,
                           float sphereRadius, float _rayLen = 1e10, LegacyVector3* pos = nullptr, LegacyVector3* normal = nullptr);

bool SphereSphereIntersection(const LegacyVector3& _sphere1Pos, float _sphere1Radius, const LegacyVector3& _sphere2Pos,
                              float _sphere2Radius);

bool SphereTriangleIntersection(const LegacyVector3& sphereCenter, float sphereRadius, const LegacyVector3& t1,
                                const LegacyVector3& t2, const LegacyVector3& t3);

bool TriTriIntersection(const LegacyVector3& v0, const LegacyVector3& v1, const LegacyVector3& v2, const LegacyVector3& u0,
                        const LegacyVector3& u1, const LegacyVector3& u2);

#endif
