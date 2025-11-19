#ifndef INCLUDED_SHAPE_H
#define INCLUDED_SHAPE_H

#include <stdio.h>

#include "LegacyVector3.h"
#include "llist.h"
#include "matrix34.h"
#include "rgb_colour.h"
#include "text_stream_readers.h"

class ShapeFragment;
class Matrix34;
class Shape;

// ****************
// Class RayPackage
// ****************

class RayPackage
{
  public:
    LegacyVector3 m_rayStart;
    LegacyVector3 m_rayEnd;
    LegacyVector3 m_rayDir;
    float m_rayLen;

    RayPackage(const LegacyVector3& _start, const LegacyVector3& _dir, float _length = 1e10)
      : m_rayStart(_start),
        m_rayDir(_dir)
    {
      m_rayLen = _length;
      m_rayEnd = m_rayStart + m_rayDir * _length;
    }
};

// *******************
// Class SpherePackage
// *******************

class SpherePackage
{
  public:
    LegacyVector3 m_pos;
    float m_radius;

    SpherePackage(const LegacyVector3& _pos, float _radius)
      : m_pos(_pos),
        m_radius(_radius) {}
};

// *****************
// Class ShapeMarker
// *****************

class ShapeMarker
{
  public:
    Matrix34 m_transform;
    std::string m_name;
    std::string m_parentName;
    int m_depth;		// Number of levels in the shape fragment tree from root to self
    ShapeFragment** m_parents;		// An array with m_depth elements

    ShapeMarker(const char* _name, const char* _parentName, int _depth, const Matrix34& _transform);
    ShapeMarker(TextReader* _in, const char* _name);
    ~ShapeMarker();

    Matrix34 GetWorldMatrix(const Matrix34& _rootTransform);
};

class VertexPosCol
{
  public:
    unsigned short m_posId;
    unsigned short m_colId;
};

// *******************
// Class ShapeTriangle
// *******************

class ShapeTriangle
{
  public:
    unsigned short v1, v2, v3;	// Vertex indices (into ShapeFragment::m_vertices)
};

// *******************
// Class ShapeFragment
// *******************

class ShapeFragment
{
  friend class ShapeExporter;

  protected:
    void ParsePositionBlock(TextReader* in, unsigned int numPositions);
    void ParseNormalBlock(TextReader* in, unsigned int numNorms);
    void ParseColorBlock(TextReader* in, unsigned int numColors);
    void ParseVertexBlock(TextReader* in, unsigned int numVerts);
    void ParseStripBlock(TextReader* in);
    void ParseAllStripBlocks(TextReader* in, unsigned int numStrips);
    void ParseTriangleBlock(TextReader* in, unsigned int numTriangles);

    void GenerateNormals();

  public:
    unsigned int m_numPositions;
    LegacyVector3* m_positions;
    LegacyVector3*
    m_positionsInWS;	// Temp storage space used to cache World Space versions of all the vertex positions in the hit check routines
    unsigned int m_numNormals;
    LegacyVector3* m_normals;
    unsigned int m_numColors;
    RGBAColor* m_colours;
    unsigned int m_numVertices;		// Each element contains an index into m_positions and an index into m_colours
    VertexPosCol* m_vertices;
    unsigned int m_numTriangles;
    unsigned int m_maxTriangles;
    ShapeTriangle* m_triangles;

    std::string m_name;
    std::string m_parentName;
    Matrix34 m_transform;
    LegacyVector3 m_angVel;
    LegacyVector3 m_vel;

    bool m_useCylinder;	// If true then use cylinder hit check instead of sphere
    LegacyVector3 m_center;
    float m_radius;
    float m_mostPositiveY;
    float m_mostNegativeY;

    LList<ShapeFragment*> m_childFragments;
    LList<ShapeMarker*> m_childMarkers;

    ShapeFragment(TextReader* _in, const char* _name);
    ShapeFragment(const char* _name, const char* _parentName);
    ~ShapeFragment();

    void RegisterPositions(LegacyVector3* positions, unsigned int numPositions);

    void Render(float _predictionTime);// Uses display list
    void RenderSlow();						// Doesn't use display list

    ShapeFragment* LookupFragment(const char* _name);	// Recurses into child fragments
    ShapeMarker* LookupMarker(const char* _name);	// Recurses into child fragments

    void CalculateCenter(const Matrix34& _transform, LegacyVector3& _center, int& _numFragments);       // Recursive
    void CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center, float& _radius);     // Recursive

    bool RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate = false);
    bool SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate = false);
    bool ShapeHit(Shape* _shape, const Matrix34& _theTransform,              // Transform of _shape
                  const Matrix34& _ourTransform,              // Transform of this
                  bool _accurate = false);
};

// ***********
// Class Shape
// ***********

class Shape
{
  protected:
    void Load(TextReader* _in);
    bool m_animating;		// If false then whole shape in one display list otherwise one display list per fragment
    const char* m_displayListName;

  public:
    ShapeFragment* m_rootFragment;
    std::string m_name;

    Shape();
    Shape(const char* filename, bool _animating);
    Shape(TextReader* in, bool _animating);
    ~Shape();

    void Render(float _predictionTime, const Matrix34& _transform);

    bool RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate = false);
    bool SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate = false);
    bool ShapeHit(Shape* _shape, const Matrix34& _theTransform,              // Transform of _shape
                  const Matrix34& _ourTransform,              // Transform of this
                  bool _accurate = false);

    LegacyVector3 CalculateCenter(const Matrix34& _transform);
    float CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center);
};

#endif
