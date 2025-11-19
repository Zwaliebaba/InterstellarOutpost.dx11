#include "pch.h"
#include "math_utils.h"
#include "LegacyVector2.h"
#include "LegacyVector3.h"
#include "matrix34.h"
#include "plane.h"

double RampUpAndDown(double _startTime, double _duration, double _timeNow)
{
  if (_timeNow > _startTime + _duration)
    return 1.0001f;

  double fractionalTime = (_timeNow - _startTime) / _duration;

  if (fractionalTime < 0.5)
  {
    double t = fractionalTime * 2.0;
    t *= t;
    t *= 0.5;
    return t;
  }
  double t = (1.0 - fractionalTime) * 2.0;
  t *= t;
  t *= 0.5;
  t = 1.0 - t;
  return t;
}

// ****************************************************************************
// Mersenne Twister Random Number Routines
// ****************************************************************************

/* This code was taken from
   http://www.math.keio.ac.jp/~matumoto/MT2002/emt19937ar.html

   C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti = N + 1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
static void init_genrand(unsigned long s)
{
  mt[0] = s & 0xffffffffUL;
  for (mti = 1; mti < N; mti++)
  {
    mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
    /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
    /* In the previous versions, MSBs of the seed affect   */
    /* only MSBs of the array mt[].                        */
    /* 2002/01/09 modified by Makoto Matsumoto             */
    mt[mti] &= 0xffffffffUL;
    /* for >32 bit machines */
  }
}

// Generates a random number on [0,0xffffffff]-interval
unsigned long syncrand()
{
  unsigned long y;
  static unsigned long mag01[2] = {0x0UL, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  if (mti >= N)
  { /* generate N words at one time */
    int kk;

    if (mti == N + 1)   /* if init_genrand() has not been called, */
      init_genrand(5489UL); /* a default initial seed is used */

    for (kk = 0; kk < N - M; kk++)
    {
      y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
      mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    for (; kk < N - 1; kk++)
    {
      y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
      mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

    mti = 0;
  }

  y = mt[mti++];

  /* Tempering */
  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);

  return y;
}

// ****************************************************************************
// General Geometry Utils
// ****************************************************************************

// Generates a Matrix34 that defines a plane, as specified by three points in the plane
void GetPlaneMatrix(const LegacyVector3& t1, const LegacyVector3& t2, const LegacyVector3& t3, Matrix34* mat)
{
  mat->f = (t1 - t2).Normalize();
  mat->r = t2 - t3;
  mat->u = (mat->f ^ mat->r).Normalize();
  mat->r = mat->f ^ mat->u;
  mat->pos = t1;
}

// Returns the distance from the point to the plane and writes the point in the plane into
// result
float ProjectPointOntoPlane(const LegacyVector3& point, const Matrix34& planeMat, LegacyVector3* result)
{
  LegacyVector3 posToCenter = point - planeMat.pos;
  float distFromCenterToPlane = posToCenter * planeMat.u;

  *result = point - planeMat.u * distFromCenterToPlane;
  return distFromCenterToPlane;
}

// Imagine that the plane is a 2D co-ordinate system embedded in a 3D co-ordinate system.
// This function takes a point in 3D space (that must line on the plane) and converts it
// into a 2D point in the "plane's co-ordinate system". If the specified point does not
// lie on the plane in the first place, then the results are undefined.
void ConvertWorldSpaceIntoPlaneSpace(const LegacyVector3& point, const Matrix34& plane, LegacyVector2* result)
{
  LegacyVector3 pointToPlaneOrigin = point - plane.pos;
  result->x = pointToPlaneOrigin * plane.r;
  result->y = pointToPlaneOrigin * plane.f;
}

// The opposite of the function above
void ConvertPlaneSpaceIntoWorldSpace(const LegacyVector2& point, const Matrix34& plane, LegacyVector3* result)
{
  *result = plane.pos + plane.f * point.y + plane.r * point.x;
}

// Returns the area of an arbitrary triangle in 2D space
float CalcTriArea(const LegacyVector2& t1, const LegacyVector2& t2, const LegacyVector2& t3)
{
  return fabsf((t2.x * t1.y - t1.x * t2.y) + (t3.x * t2.y - t2.x * t3.y) + (t1.x * t3.y - t3.x * t1.y)) * 0.5f;
}

// ****************************************************************************
// 2D Intersection Tests
// ****************************************************************************

bool IsPointInTriangle(const LegacyVector2& pos, const LegacyVector2& t1, const LegacyVector2& t2, const LegacyVector2& t3)
{
  float triArea = CalcTriArea(t1, t2, t3) * 1.0001f;
  float subTrisArea = CalcTriArea(t1, t2, pos);
  if (subTrisArea > triArea)
    return false;
  subTrisArea += CalcTriArea(t2, t3, pos);
  if (subTrisArea > triArea)
    return false;
  subTrisArea += CalcTriArea(t3, t1, pos);
  if (subTrisArea > triArea)
    return false;
  return true;
}

// Finds the point on the line segment that is nearest to the specified point.
// Often this will be one of the end points of the line segment
float PointSegDist2D(const LegacyVector2& p,	// Point
                     const LegacyVector2& l0, const LegacyVector2& l1, // Line seg
                     LegacyVector2* result)
{
  // Get direction of line
  LegacyVector2 v = l1 - l0;

  // Get vector from start of line to point
  LegacyVector2 w = p - l0;

  // Compute w dot v;
  float c1 = w.x * v.x + w.y * v.y;

  // If c1 <= 0.0f then the end point l0 is the nearest to p
  if (c1 <= 0.0f)
  {
    if (result)
      *result = l0;
    LegacyVector2 delta = l0 - p;
    return delta.Mag();
  }

  // Compute length squared of v, equivalent to v dot v (a dot b = |a| |b| cos theta)
  float c2 = v.MagSquared();

  // If c2 <= c1 then the end point l1 is the nearest to p
  if (c2 <= c1)
  {
    if (result)
      *result = l1;
    LegacyVector2 delta = l1 - p;
    return delta.Mag();
  }

  // Otherwise the nearest point is somewhere along the segment
  float b = c1 / c2;
  if (result)
    *result = l0 + b * v;

  LegacyVector2 delta = (l0 + b * v) - p;
  return delta.Mag();
}

// Adapted from comp.graphics.algorithms FAQ item 1.03
bool SegRayIntersection2D(const LegacyVector2& _lineStart, const LegacyVector2& _lineEnd, const LegacyVector2& _rayStart,
                          const LegacyVector2& _rayDir, LegacyVector2* _result)
{
  float r = (_lineStart.y - _rayStart.y) * _rayDir.x - (_lineStart.x - _rayStart.x) * _rayDir.y;
  r /= (_lineEnd.x - _lineStart.x) * _rayDir.y - (_lineEnd.y - _lineStart.y) * _rayDir.x;

  float s = (_lineStart.y - _rayStart.y) * (_lineEnd.x - _lineStart.x) - (_lineStart.x - _rayStart.x) * (_lineEnd.y - _lineStart.y);
  s /= (_lineEnd.x - _lineStart.x) * _rayDir.y - (_lineEnd.y - _lineStart.y) * _rayDir.x;

  if (r >= 0.0f && r <= 1.0f && s >= 0.0f)
  {
    if (_result)
      *_result = _rayStart + _rayDir * s;

    return true;
  }

  return false;
}

// ****************************************************************************
// 3D Intersection Tests
// ****************************************************************************

// Returns the distance between to infinite 3D lines, assuming that they are skew.
// Stores the points of closest approach in posOnA and posOnB
float RayRayDist(const LegacyVector3& a, const LegacyVector3& aDir, const LegacyVector3& b, const LegacyVector3& bDir,
                 LegacyVector3* posOnA, LegacyVector3* posOnB)
{
  // Connecting line is perpendicular to both
  LegacyVector3 cDir = aDir ^ bDir;
  LegacyVector3 temp1, temp2;

  if (posOnA == nullptr)
    posOnA = &temp1;
  if (posOnB == nullptr)
    posOnB = &temp2;

  // Check for near-parallel lines
  //    if ( 0 )
  //	{
  //        return 0.0f;	// degenerate case: lines are parallel
  //    }

  // Form plane containing line A, parallel to cdir
  Plane planeA(a, a + aDir, a + cDir);

  // Form plane containing line B, parallel to cdir
  Plane planeB(b, b + bDir, b + cDir);

  // Now we are going to find the points in the two
  {
    // Where line A intersects planeB is the point of closest approach to line B
    int result1 = RayPlaneIntersection(a, aDir, planeB, posOnA);

    // Where line B intersects planeA is the point of closest approach to line A
    int result2 = RayPlaneIntersection(b, bDir, planeA, posOnB);
  }

  float dist = ((*posOnA) - (*posOnB)).Mag();

  return dist;
}

// Returns the distance between an infinite 3D line and a 3D line segment.
// Store the points of closest approach in posOnRay and posInSeg
float RaySegDist(const LegacyVector3& pointOnLine, const LegacyVector3& lineDir, const LegacyVector3& segStart,
                 const LegacyVector3& segEnd, LegacyVector3* posOnRay, LegacyVector3* posInSeg)
{
  LegacyVector3 segDir = segEnd - segStart;

  LegacyVector3 temp1, temp2;
  if (!posOnRay)
    posOnRay = &temp1;
  if (!posInSeg)
    posInSeg = &temp2;

  float dist = RayRayDist(pointOnLine, lineDir, segStart, segDir, posOnRay, posInSeg);

  float t = 0.0f;

  if (segDir.x > 0.0001f)
    t = (posInSeg->x - segStart.x) / segDir.x;
  else if (segDir.y > 0.0001f)
    t = (posInSeg->y - segStart.y) / segDir.y;
  else
    t = (posInSeg->z - segStart.z) / segDir.z;

  if (t > 0.0f && t < 1.0f)
    return dist;

  return dist * 100000.0f;
}

bool RayTriIntersection(const LegacyVector3& orig, const LegacyVector3& dir, const LegacyVector3& vert0, const LegacyVector3& vert1,
                        const LegacyVector3& vert2, float _rayLen, LegacyVector3* _result)
{
  LegacyVector3 edge1, edge2, tvec, pvec, qvec;
  float det, inv_det;

  // Find vectors for two edges sharing vert0
  edge1 = vert1 - vert0;
  edge2 = vert2 - vert0;

  // Begin calculating determinant - also used to calculate U parameter
  pvec = dir ^ edge2;

  // If determinant is near zero, ray lies in plane of triangle
  det = edge1 * pvec;

  if (det > -0.0000001f && det < 0.0000001f)
    return false;
  inv_det = 1.0f / det;

  /* calculate distance from vert0 to ray origin */
  tvec = orig - vert0;

  LegacyVector3 result;

  /* calculate Y parameter and test bounds */
  result.y = (tvec * pvec) * inv_det;
  if (result.y < 0.0f || result.y > 1.0f)
    return false;

  /* prepare to test Z parameter */
  qvec = tvec ^ edge1;

  /* calculate Z parameter and test bounds */
  result.z = (dir * qvec) * inv_det;
  if (result.z < 0.0f || result.y + result.z > 1.0f)
    return false;

  /* calculate X, ray intersects triangle */
  result.x = (edge2 * qvec) * inv_det;

  //    if (result.x > _rayLen ) return false;
  if (result.MagSquared() > _rayLen * _rayLen)
    return false;

  if (_result)
    *_result = orig + dir * result.x;

  return true;
}

bool XM_CALLCONV RaySphereIntersection(XMVECTOR rayStart, XMVECTOR rayDir, XMVECTOR spherePos,
                           float sphereRadius, float _rayLen, LegacyVector3* pos, LegacyVector3* normal)
{
  XMVECTOR l = spherePos - rayStart;

  // Find tca the distance along ray of point nearest to sphere center.
  // We'll call this point P
  float tca = Dotf(l, rayDir);
  if (tca < 0.0f)
    return false;

  // Use Pythagoras now to find dist from P to sphere center. Actually
  // cheaper to calc dist sqrd and compare to radius sqrd
  float radiusSqrd = sphereRadius * sphereRadius;
  float lMagSqrd =  LengthSquare(l);
  float d2 = lMagSqrd - (tca * tca);
  if (d2 > radiusSqrd)
    return false;

  float thc = sqrtf(radiusSqrd - d2);
  float t = tca - thc;

  if (t < 0 || t > _rayLen)
    return false;

  if (pos)
    *pos = rayStart + rayDir * t;

  if (normal)
  {
    *normal = *pos - LegacyVector3(spherePos);
    normal->Normalize();
  }

  return true;
}

//bool RayPlaneIntersection( LegacyVector3 const &rayStart, LegacyVector3 const &rayDir,
////                           LegacyVector3 const &planePos, LegacyVector3 const &planeNormal,
//						   Plane const &plane,
//                           float _rayLen, LegacyVector3 *pos )
//{
//	LegacyVector3 pointInPlane = plane.m_distFromOrigin * plane.m_normal;
//    float d = -pointInPlane.y;
//    float t = -( rayStart * plane.m_normal + d ) / ( rayDir * plane.m_normal );
//    if( t < 0.0f ) return false;
//    if( t > _rayLen ) return false;
//
//    if (pos)
//	{
//		*pos = rayStart + rayDir * t;
//	}
//
//    return true;
//}

// Returns 0 if line is parallel to plane but doens't intersect,
//		   1 if line is in plane (ie intersects everywhere),
//		   2 otherwise (this is the general case)
int RayPlaneIntersection(const LegacyVector3& pOnLine, const LegacyVector3& lineDir, const Plane& plane,
                         LegacyVector3* intersectionPoint)
{
  /*If the plane is defined as:
         a*x + b*y + c*z + d = 0
  and the line is defined as:
        x = x1 + (x2 - x1)*t = x1 + i*t
        y = y1 + (y2 - y1)*t = y1 + j*t
        z = z1 + (z2 - z1)*t = z1 + k*t
  Then just substitute these into the plane equation. You end up
    with:
    t = - (a*x1 + b*y1 + c*z1 + d)/(a*i + b*j + c*k)
  When the denominator is zero, the line is contained in the plane
    if the numerator is also zero (the point at t=0 satisfies the
    plane equation), otherwise the line is parallel to the plane.*/

  float a, b, c, d;
  plane.GetCartesianDefinition(&a, &b, &c, &d);

  float x1 = pOnLine.x;
  float y1 = pOnLine.y;
  float z1 = pOnLine.z;

  float i = lineDir.x;
  float j = lineDir.y;
  float k = lineDir.z;

  float numerator = -(a * x1 + b * y1 + c * z1 + d);
  float denominator = a * i + b * j + c * k;

  if (fabs(denominator) < 1e-7)
  {
    if (fabs(numerator) < 1e-7)
      return 1;
    return 0;
  }
  float t = numerator / denominator;

  intersectionPoint->x = x1 + i * t;
  intersectionPoint->y = y1 + j * t;
  intersectionPoint->z = z1 + k * t;

  return 2;
}

bool SphereSphereIntersection(const LegacyVector3& _sphere1Pos, float _sphere1Radius, const LegacyVector3& _sphere2Pos,
                              float _sphere2Radius)
{
  float distanceSqrd = (_sphere1Pos - _sphere2Pos).MagSquared();
  float radiiSummed = _sphere1Radius + _sphere2Radius;
  return (distanceSqrd <= radiiSummed * radiiSummed);
}

bool SphereTriangleIntersection(const LegacyVector3& sphereCenter, float sphereRadius, const LegacyVector3& t1,
                                const LegacyVector3& t2, const LegacyVector3& t3)
{
  Matrix34 planeMat;
  GetPlaneMatrix(t1, t2, t3, &planeMat);

  LegacyVector3 result;
  float dist = ProjectPointOntoPlane(sphereCenter, planeMat, &result);
  if (dist > sphereRadius)
    return false;

  //	DrawPoint(result);

  LegacyVector2 point2D;
  ConvertWorldSpaceIntoPlaneSpace(result, planeMat, &point2D);

  LegacyVector2 t12D, t22D, t32D;
  ConvertWorldSpaceIntoPlaneSpace(t1, planeMat, &t12D);
  ConvertWorldSpaceIntoPlaneSpace(t2, planeMat, &t22D);
  ConvertWorldSpaceIntoPlaneSpace(t3, planeMat, &t32D);

  bool isPointInTri = IsPointInTriangle(point2D, t12D, t22D, t32D);

  if (!isPointInTri)
  {
    // Test against edge 1
    LegacyVector2 pointInTriangle;
    float nearest = PointSegDist2D(point2D, t12D, t22D, &pointInTriangle);

    // Test against edge 2
    LegacyVector2 temp;
    float thisDist = PointSegDist2D(point2D, t22D, t32D, &temp);
    if (thisDist < nearest)
    {
      nearest = thisDist;
      pointInTriangle = temp;
    }

    // Test against edge 3
    thisDist = PointSegDist2D(point2D, t32D, t12D, &temp);
    if (thisDist < nearest)
    {
      nearest = thisDist;
      pointInTriangle = temp;
    }

    ConvertPlaneSpaceIntoWorldSpace(pointInTriangle, planeMat, &result);
    //		DrawPoint(result);
  }

  LegacyVector3 centerToNearestPointInPlane = sphereCenter - result;
  if (centerToNearestPointInPlane.MagSquared() > sphereRadius * sphereRadius)
    return false;

  return true;
}
