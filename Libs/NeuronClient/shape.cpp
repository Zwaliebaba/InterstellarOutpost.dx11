#include "pch.h"
#include "shape.h"
#include "GameApp.h"
#include "math_utils.h"
#include "matrix34.h"
#include "text_stream_readers.h"

// This constructor is used in the export process. The m_parents array is never
// populated in the exporter, so it is intentionally left blank
ShapeMarker::ShapeMarker(const char* _name, const char* _parentName, int _depth, const Matrix34& _transform)
  : m_transform(_transform),
    m_depth(_depth),
    m_parents(nullptr)
{
  m_name = _name;
  m_parentName = _parentName;

  std::erase(m_name, ' ');
  std::erase(m_parentName, ' ');
}

ShapeMarker::ShapeMarker(TextReader* _in, const char* _name)
{
  m_name = _name;
  m_parentName.clear();

  while (_in->ReadLine())
  {
    if (char* firstWord = _in->GetNextToken())
    {
      if (_stricmp(firstWord, "ParentName") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_parentName = _strupr(secondWord);
      }
      else if (_stricmp(firstWord, "Depth") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_depth = atoi(secondWord);
      }
      else if (_stricmp(firstWord, "front") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_transform.f.x = static_cast<float>(atof(secondWord));
        m_transform.f.y = static_cast<float>(atof(_in->GetNextToken()));
        m_transform.f.z = static_cast<float>(atof(_in->GetNextToken()));
      }
      else if (_stricmp(firstWord, "up") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_transform.u.x = static_cast<float>(atof(secondWord));
        m_transform.u.y = static_cast<float>(atof(_in->GetNextToken()));
        m_transform.u.z = static_cast<float>(atof(_in->GetNextToken()));
      }
      else if (_stricmp(firstWord, "pos") == 0)
      {
        char* secondWord = _in->GetNextToken();
        m_transform.pos.x = static_cast<float>(atof(secondWord));
        m_transform.pos.y = static_cast<float>(atof(_in->GetNextToken()));
        m_transform.pos.z = static_cast<float>(atof(_in->GetNextToken()));
      }
      else
        if (_stricmp(firstWord, "MarkerEnd") == 0) { break; }
    }
  }

  m_transform.Normalize();

  m_parents = NEW ShapeFragment*[m_depth];

  if (m_name.empty())
    m_name = "unknown";
  if (m_parentName.empty())
    m_parentName = "unknown";
}

ShapeMarker::~ShapeMarker()
{
  SAFE_DELETE(m_parents);
}

// *** GetWorldMatrix
Matrix34 ShapeMarker::GetWorldMatrix(const Matrix34& _rootTransform)
{
  Matrix34 mat = _rootTransform;
  for (int i = 0; i < m_depth; ++i) { mat = m_parents[i]->m_transform * mat; }
  mat = m_transform * mat;

  return mat;
}

// This constructor is used to load a shape from a file.
ShapeFragment::ShapeFragment(TextReader* _in, const char* _name)
  : m_numPositions(0),
    m_positions(nullptr),
    m_positionsInWS(nullptr),
    m_numNormals(0),
    m_normals(nullptr),
    m_numColors(0),
    m_colours(nullptr),
    m_numVertices(0),
    m_vertices(nullptr),
    m_numTriangles(0),
    m_maxTriangles(0),
    m_triangles(nullptr),
    m_transform(1),
    m_angVel(0, 0, 0),
    m_vel(0, 0, 0),
    m_useCylinder(false),
    m_center(0.0f, 0.0f, 0.0f),
    m_radius(-1.0f),
    m_mostPositiveY(0.0f),
    m_mostNegativeY(0.0f)
{
  m_maxTriangles = 1;
  m_triangles = NEW ShapeTriangle[m_maxTriangles];

  DEBUG_ASSERT(_name);
  m_name = _name;

  m_transform.SetToIdentity();
  m_angVel.Zero();
  m_vel.Zero();

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;

    char* firstWord = _in->GetNextToken();
    char* secondWord = _in->GetNextToken();

    if (_stricmp(firstWord, "ParentName") == 0) { m_parentName = _strupr(secondWord); }
    else if (_stricmp(firstWord, "front") == 0)
    {
      m_transform.f.x = static_cast<float>(atof(secondWord));
      m_transform.f.y = static_cast<float>(atof(_in->GetNextToken()));
      m_transform.f.z = static_cast<float>(atof(_in->GetNextToken()));
    }
    else if (_stricmp(firstWord, "up") == 0)
    {
      m_transform.u.x = static_cast<float>(atof(secondWord));
      m_transform.u.y = static_cast<float>(atof(_in->GetNextToken()));
      m_transform.u.z = static_cast<float>(atof(_in->GetNextToken()));
    }
    else if (_stricmp(firstWord, "pos") == 0)
    {
      m_transform.pos.x = static_cast<float>(atof(secondWord));
      m_transform.pos.y = static_cast<float>(atof(_in->GetNextToken()));
      m_transform.pos.z = static_cast<float>(atof(_in->GetNextToken()));
    }
    else if (_stricmp(firstWord, "Positions") == 0)
    {
      int numPositions = atoi(secondWord);
      ParsePositionBlock(_in, numPositions);
    }
    else if (_stricmp(firstWord, "Normals") == 0)
    {
      int numNorms = atoi(secondWord);
      ParseNormalBlock(_in, numNorms);
    }
    else if (_stricmp(firstWord, "Colors") == 0)
    {
      int numColors = atoi(secondWord);
      ParseColorBlock(_in, numColors);
    }
    else if (_stricmp(firstWord, "Vertices") == 0)
    {
      int numVerts = atoi(secondWord);
      ParseVertexBlock(_in, numVerts);
    }
    else if (_stricmp(firstWord, "Strips") == 0)
    {
      int numStrips = atoi(secondWord);
      ParseAllStripBlocks(_in, numStrips);
      break;
    }
    else if (_stricmp(firstWord, "Triangles") == 0)
    {
      int numTriangles = atoi(secondWord);
      ParseTriangleBlock(_in, numTriangles);
      break;
    }
  }

  m_transform.Normalize();

  if (m_name.empty())
    m_name = "unknown";
  if (m_parentName.empty())
    m_parentName = "unknown";

  GenerateNormals();

  m_positionsInWS = NEW LegacyVector3[m_numVertices];
}

// This constructor is used when you want to build a shape from scratch yourself,
// eg in the exporter.
ShapeFragment::ShapeFragment(const char* _name, const char* _parentName)
  : m_numPositions(0),
    m_positions(nullptr),
    m_positionsInWS(nullptr),
    m_numNormals(0),
    m_normals(nullptr),
    m_numColors(0),
    m_colours(nullptr),
    m_numVertices(0),
    m_vertices(nullptr),
    m_numTriangles(0),
    m_maxTriangles(0),
    m_triangles(nullptr),
    m_useCylinder(false),
    m_center(0.0f, 0.0f, 0.0f),
    m_radius(-1.0f),
    m_mostPositiveY(0.0f),
    m_mostNegativeY(0.0f)
{
  m_maxTriangles = 1;
  m_triangles = NEW ShapeTriangle[m_maxTriangles];

  m_transform.SetToIdentity();
  m_angVel.Zero();
  m_vel.Zero();
  m_name = _name;
  m_parentName = _parentName;

  // Remove spaces from m_name
  char* c = m_name.data();
  char* d = m_name.data();

  while (d[0] != '\0')
  {
    c[0] = d[0];
    if (c[0] != ' ') { c++; }
    d++;
  }
  c[0] = '\0';

  // Remove spaces from m_parentName
  c = m_parentName.data();
  d = m_parentName.data();

  while (d[0] != '\0')
  {
    c[0] = d[0];
    if (c[0] != ' ') { c++; }
    d++;
  }
  c[0] = '\0';
}

ShapeFragment::~ShapeFragment()
{
  SAFE_DELETE_ARRAY(m_positions);
  SAFE_DELETE_ARRAY(m_positionsInWS);
  delete [] m_vertices;
  m_vertices = nullptr;
  delete [] m_normals;
  m_normals = nullptr;
  delete [] m_colours;
  m_colours = nullptr;
  delete [] m_triangles;
  m_triangles = nullptr;
  m_childFragments.EmptyAndDelete();
  m_childMarkers.EmptyAndDelete();
}

void ShapeFragment::ParsePositionBlock(TextReader* _in, unsigned int _numPositions)
{
  auto positions = NEW LegacyVector3[_numPositions];

  int expectedId = 0;
  while (expectedId < _numPositions)
  {
    if (_in->ReadLine() == 0) { DEBUG_ASSERT(0); }

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      int id = atoi(c);
      if (id != expectedId || id >= _numPositions) { return; }

      LegacyVector3* vect = &positions[id];
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->x = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->y = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->z = static_cast<float>(atof(c));

      expectedId++;
    }
  }

  RegisterPositions(positions, _numPositions);
}

// *** ParseNormalBlock
void ShapeFragment::ParseNormalBlock(TextReader* _in, unsigned int _numNorms)
{
  if (_numNorms != 0) { m_normals = NEW LegacyVector3[_numNorms]; }
  m_numNormals = _numNorms;

  int expectedId = 0;
  while (expectedId < _numNorms)
  {
    if (_in->ReadLine() == 0) { DEBUG_ASSERT(0); }

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      int id = atoi(c);
      if (id != expectedId || id >= _numNorms) { DEBUG_ASSERT(0); }

      LegacyVector3* vect = &m_normals[id];
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->x = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->y = static_cast<float>(atof(c));
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      vect->z = static_cast<float>(atof(c));

      expectedId++;
    }
  }
}

// *** ParseColorBlock
void ShapeFragment::ParseColorBlock(TextReader* _in, unsigned int _numColors)
{
  m_colours = NEW RGBAColor[_numColors];
  m_numColors = _numColors;

  int expectedId = 0;
  while (expectedId < _numColors)
  {
    if (_in->ReadLine() == 0) { DEBUG_ASSERT(0); }

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      int id = atoi(c);
      if (id != expectedId || id >= _numColors) { DEBUG_ASSERT(0); }

      RGBAColor* col = &m_colours[id];
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      col->r = atoi(c);
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      col->g = atoi(c);
      c = _in->GetNextToken();
      DEBUG_ASSERT(c);
      col->b = atoi(c);
      col->a = 0;

      *col *= 0.85f;
      col->a = 255;

      expectedId++;
    }
  }
}

// *** ParseVertexBlock
void ShapeFragment::ParseVertexBlock(TextReader* _in, unsigned int _numVerts)
{
  m_vertices = NEW VertexPosCol[_numVerts];
  m_numVertices = _numVerts;

  int expectedId = 0;
  while (expectedId < _numVerts)
  {
    if (_in->ReadLine() == 0) { DEBUG_ASSERT(0); }

    char* c = _in->GetNextToken();
    if (c && isdigit(c[0]))
    {
      int id = atoi(c);
      if (id != expectedId || id >= _numVerts) { DEBUG_ASSERT(0); }

      VertexPosCol* vert = &m_vertices[id];
      c = _in->GetNextToken();
      vert->m_posId = atoi(c);
      c = _in->GetNextToken();
      vert->m_colId = atoi(c);

      expectedId++;
    }
  }
}

// *** ParseStripBlock
void ShapeFragment::ParseStripBlock(TextReader* _in)
{
  _in->ReadLine();
  char* c = _in->GetNextToken();
  DEBUG_ASSERT(c);

  // Read material name
  if (_stricmp(c, "Material") == 0)
  {
    c = _in->GetNextToken();
    DEBUG_ASSERT(c);

    _in->ReadLine();
    c = _in->GetNextToken();
  }

  // Read number of vertices in strip
  DEBUG_ASSERT(c && (_stricmp(c, "Verts") == 0));
  c = _in->GetNextToken();
  DEBUG_ASSERT(c);
  int numVerts = atoi(c);
  DEBUG_ASSERT(numVerts > 2);

  // Now just read a sequence of verts
  int i = 0;
  int v1 = -1, v2 = -1;
  while (i < numVerts)
  {
    if (_in->ReadLine() == 0) { DEBUG_ASSERT(0); }

    while (_in->TokenAvailable())
    {
      char* c = _in->GetNextToken();
      DEBUG_ASSERT(c[0] == 'v');

      c++;
      int v3 = atoi(c);
      DEBUG_ASSERT(v3 < m_numVertices);

      if (i >= 2 && v1 != v2 && v2 != v3 && v1 != v3)
      {
        if (m_numTriangles == m_maxTriangles)
        {
          auto newTriangles = NEW ShapeTriangle[m_maxTriangles * 2];
          memcpy(newTriangles, m_triangles, sizeof(ShapeTriangle) * m_maxTriangles);
          delete [] m_triangles;
          m_triangles = newTriangles;
          m_maxTriangles *= 2;
        }
        if (i & 1)
        {
          m_triangles[m_numTriangles].v1 = v3;
          m_triangles[m_numTriangles].v2 = v2;
          m_triangles[m_numTriangles].v3 = v1;
        }
        else
        {
          m_triangles[m_numTriangles].v1 = v1;
          m_triangles[m_numTriangles].v2 = v2;
          m_triangles[m_numTriangles].v3 = v3;
        }
        m_numTriangles++;
      }

      v1 = v2;
      v2 = v3;

      i++;
    }
  }
}

// *** ParseAllStripBlocks
void ShapeFragment::ParseAllStripBlocks(TextReader* _in, unsigned int _numStrips)
{
  int expectedId = 0;
  while (_in->ReadLine())
  {
    char* c = _in->GetNextToken();
    if (c && _stricmp(c, "Strip") == 0)
    {
      c = _in->GetNextToken();
      int id = atoi(c);

      DEBUG_ASSERT(id == expectedId);

      ParseStripBlock(_in);

      expectedId++;

      if (expectedId == _numStrips) { break; }
    }
  }

  // Shrink m_triangles to tightly fit its data
  auto newTriangles = NEW ShapeTriangle[m_numTriangles];
  memcpy(newTriangles, m_triangles, sizeof(ShapeTriangle) * m_numTriangles);
  delete [] m_triangles;
  m_maxTriangles = m_numTriangles;
  m_triangles = newTriangles;
}

void ShapeFragment::ParseTriangleBlock(TextReader* _in, unsigned int _numTriangles)
{
  DEBUG_ASSERT(m_numTriangles == 0 && m_maxTriangles == 1 && m_triangles != NULL);
  delete [] m_triangles;

  m_maxTriangles = _numTriangles;
  m_triangles = NEW ShapeTriangle[_numTriangles];

  while (m_numTriangles < _numTriangles)
  {
    _in->ReadLine();
    char* c = _in->GetNextToken();
    m_triangles[m_numTriangles].v1 = atoi(c);
    c = _in->GetNextToken();
    m_triangles[m_numTriangles].v2 = atoi(c);
    c = _in->GetNextToken();
    m_triangles[m_numTriangles].v3 = atoi(c);

    m_numTriangles++;
  }
}

// *** GenerateNormals
// Currently this function generates one normal for each face in all the
// strips, rather than one normal per vertex. This is what we need for
// facet shading, rather than smooth (gouraud) shading.
void ShapeFragment::GenerateNormals()
{
  m_numNormals = m_numTriangles;
  m_normals = NEW LegacyVector3[m_numNormals];
  int normId = 0;

  for (int j = 0; j < m_numTriangles; ++j)
  {
    ShapeTriangle* tri = &m_triangles[j];
    const VertexPosCol& vertA = m_vertices[tri->v1];
    const VertexPosCol& vertB = m_vertices[tri->v2];
    const VertexPosCol& vertC = m_vertices[tri->v3];
    LegacyVector3& a = m_positions[vertA.m_posId];
    LegacyVector3& b = m_positions[vertB.m_posId];
    LegacyVector3& c = m_positions[vertC.m_posId];
    LegacyVector3 ab = b - a;
    LegacyVector3 bc = c - b;
    m_normals[normId] = ab ^ bc;
    m_normals[normId].Normalize();
    //		if (!(j & 1)) m_normals[normId] = -m_normals[normId];
    normId++;
  }
}

// *** RegisterPositions
void ShapeFragment::RegisterPositions(LegacyVector3* _positions, unsigned int _numPositions)
{
  int i;

  delete [] m_positions;
  m_positions = _positions;
  m_numPositions = _numPositions;

  // Calculate bounding sphere
  {
    // Find the center of the fragment
    {
      float minX = FLT_MAX;
      float maxX = -FLT_MAX;
      float minY = FLT_MAX;
      float maxY = -FLT_MAX;
      float minZ = FLT_MAX;
      float maxZ = -FLT_MAX;
      for (i = 0; i < m_numPositions; ++i)
      {
        minX = std::min(m_positions[i].x, minX);
        maxX = std::max(m_positions[i].x, maxX);
        minY = std::min(m_positions[i].y, minY);
        maxY = std::max(m_positions[i].y, maxY);
        minZ = std::min(m_positions[i].z, minZ);
        maxZ = std::max(m_positions[i].z, maxZ);
      }
      m_center.x = (maxX + minX) / 2;
      m_center.y = (maxY + minY) / 2;
      m_center.z = (maxZ + minZ) / 2;
    }

    // Find the point furthest from the center
    float radiusSquared = 0.0f;
    for (i = 0; i < m_numPositions; ++i)
    {
      LegacyVector3 delta = m_center - m_positions[i];
      float magSquared = delta.MagSquared();
      radiusSquared = std::max(magSquared, radiusSquared);
    }
    m_radius = sqrtf(radiusSquared);
  }

  // Calculate bounding vertical cylinder
  {
    // Find the vertical extents of the fragment and minimum enclosing
    // cylinder radius
    float radiusSquared = 0.0f;
    m_mostPositiveY = -FLT_MAX;
    m_mostNegativeY = FLT_MAX;
    for (i = 0; i < m_numPositions; ++i)
    {
      m_mostPositiveY = std::max(m_positions[i].y, m_mostPositiveY);
      m_mostNegativeY = std::min(m_positions[i].y, m_mostNegativeY);
      LegacyVector3 delta = m_center - m_positions[i];
      delta.y = 0.0f;
      radiusSquared = std::max(delta.MagSquared(), radiusSquared);
    }
  }
}

void ShapeFragment::Render(float _predictionTime)
{
  Matrix34 predictedTransform = m_transform;
  predictedTransform.RotateAround(m_angVel * _predictionTime);
  predictedTransform.pos += m_vel * _predictionTime;

  bool matrixIsIdentity = predictedTransform == g_identityMatrix34;
  if (!matrixIsIdentity)
  {
    glPushMatrix();
    glMultMatrixf(predictedTransform.ConvertToOpenGLFormat());
  }

  RenderSlow();

  int numChildren = m_childFragments.Size();
  for (int i = 0; i < numChildren; ++i) { m_childFragments.GetData(i)->Render(_predictionTime); }

  if (!matrixIsIdentity) { glPopMatrix(); }
}

void ShapeFragment::RenderSlow()
{
  if (!m_numTriangles)
    return;
  glBegin(GL_TRIANGLES);

  int norm = 0;
  for (int i = 0; i < m_numTriangles; i++)
  {
    const VertexPosCol* vertA = &m_vertices[m_triangles[i].v1];
    const VertexPosCol* vertB = &m_vertices[m_triangles[i].v2];
    const VertexPosCol* vertC = &m_vertices[m_triangles[i].v3];

    constexpr unsigned char alpha = 255;

    glNormal3fv(m_normals[norm].GetData());
    glColor4ub(m_colours[vertA->m_colId].r, m_colours[vertA->m_colId].g, m_colours[vertA->m_colId].b, alpha);
    glVertex3fv(m_positions[vertA->m_posId].GetData());

    glNormal3fv(m_normals[norm].GetData());
    glColor4ub(m_colours[vertB->m_colId].r, m_colours[vertB->m_colId].g, m_colours[vertB->m_colId].b, alpha);
    glVertex3fv(m_positions[vertB->m_posId].GetData());

    glNormal3fv(m_normals[norm].GetData());
    glColor4ub(m_colours[vertC->m_colId].r, m_colours[vertC->m_colId].g, m_colours[vertC->m_colId].b, alpha);
    glVertex3fv(m_positions[vertC->m_posId].GetData());
    norm++;
  }
  glEnd();
}

// *** LookupFragment
// Recursively look through all child fragments until we find a name match
ShapeFragment* ShapeFragment::LookupFragment(const char* _name)
{
  if (_stricmp(_name, m_name.c_str()) == 0) { return this; }
  int numChildFragments = m_childFragments.Size();
  for (int i = 0; i < numChildFragments; ++i)
  {
    ShapeFragment* frag = m_childFragments.GetData(i)->LookupFragment(_name);
    if (frag) { return frag; }
  }

  return nullptr;
}

// *** LookupMarker
// Recursively look through all child fragments until we find one with a marker
// matching the specified name
ShapeMarker* ShapeFragment::LookupMarker(const char* _name)
{
  int i;

  int numMarkers = m_childMarkers.Size();
  for (i = 0; i < numMarkers; ++i)
  {
    ShapeMarker* marker = m_childMarkers.GetData(i);
    if (_stricmp(_name, marker->m_name.c_str()) == 0) { return marker; }
  }

  int numChildFragments = m_childFragments.Size();
  for (i = 0; i < numChildFragments; ++i)
  {
    ShapeMarker* marker = m_childFragments.GetData(i)->LookupMarker(_name);
    if (marker) { return marker; }
  }

  return nullptr;
}

// *** RayHit
bool ShapeFragment::RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate)
{
  Matrix34 totalMatrix = m_transform * _transform;
  LegacyVector3 center = totalMatrix * m_center;

  // First do bounding sphere check
  if (m_radius > 0.0f && RaySphereIntersection(_package->m_rayStart, _package->m_rayDir, center, m_radius, _package->m_rayLen))
  {
    // Exit early to save loads of work if we don't care about accuracy too much
    if (_accurate == false) { return true; }

    // Compute World Space versions of all the vertices
    for (int i = 0; i < m_numPositions; ++i) { m_positionsInWS[i] = m_positions[i] * totalMatrix; }

    // Check each triangle in this fragment for intersection
    for (int j = 0; j < m_numTriangles; ++j)
    {
      VertexPosCol* v1 = &m_vertices[m_triangles[j].v1];
      VertexPosCol* v2 = &m_vertices[m_triangles[j].v2];
      VertexPosCol* v3 = &m_vertices[m_triangles[j].v3];
      if (RayTriIntersection(_package->m_rayStart, _package->m_rayDir, m_positionsInWS[v1->m_posId], m_positionsInWS[v2->m_posId],
                             m_positionsInWS[v3->m_posId])) { return true; }
    }
  }

  // If we haven't found a hit then recurse into all child fragments
  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragment* frag = m_childFragments.GetData(i);
    //		if (frag->RayHit(&package, totalMatrix))
    if (frag->RayHit(_package, totalMatrix, _accurate)) { return true; }
  }
  return false;
}

// *** SphereHit
bool ShapeFragment::SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate)
{
  Matrix34 totalMatrix = m_transform * _transform;
  LegacyVector3 center = totalMatrix * m_center;

  if (m_radius > 0.0f && SphereSphereIntersection(_package->m_pos, _package->m_radius, center, m_radius))
  {
    // Exit early to save loads of work if we don't care about accuracy too much
    if (_accurate == false) { return true; }

    // Compute World Space versions of all the vertices
    for (int i = 0; i < m_numPositions; ++i) { m_positionsInWS[i] = m_positions[i] * totalMatrix; }

    // Check each triangle in this fragment for intersection
    for (int j = 0; j < m_numTriangles; ++j)
    {
      VertexPosCol* v1 = &m_vertices[m_triangles[j].v1];
      VertexPosCol* v2 = &m_vertices[m_triangles[j].v2];
      VertexPosCol* v3 = &m_vertices[m_triangles[j].v3];
      if (SphereTriangleIntersection(_package->m_pos, _package->m_radius, m_positionsInWS[v1->m_posId],
                                     m_positionsInWS[v2->m_posId], m_positionsInWS[v3->m_posId])) { return true; }
    }
  }

  // If we haven't found a hit then recurse into all child fragments
  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragment* frag = m_childFragments.GetData(i);
    if (frag->SphereHit(_package, totalMatrix, _accurate)) { return true; }
  }
  return false;
}

// *** ShapeHit
bool ShapeFragment::ShapeHit(Shape* _shape, const Matrix34& _theTransform, const Matrix34& _ourTransform, bool _accurate)
{
  Matrix34 totalMatrix = m_transform * _ourTransform;
  LegacyVector3 center = totalMatrix * m_center;

  if (m_radius > 0.0f)
  {
    SpherePackage package(center, m_radius);
    if (_shape->SphereHit(&package, _theTransform, _accurate)) { return true; }
  }

  int i;

  // If we haven't found a hit then recurse into all child fragments
  int numFragments = m_childFragments.Size();
  for (i = 0; i < numFragments; ++i)
  {
    ShapeFragment* frag = m_childFragments.GetData(i);
    if (frag->ShapeHit(_shape, _theTransform, totalMatrix, _accurate)) { return true; }
  }
  return false;
}

void ShapeFragment::CalculateCenter(const Matrix34& _transform, LegacyVector3& _center, int& _numFragments)
{
  Matrix34 totalMatrix = m_transform * _transform;
  LegacyVector3 center = totalMatrix * m_center;

  _center += center;
  _numFragments++;

  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragment* frag = m_childFragments.GetData(i);
    frag->CalculateCenter(totalMatrix, _center, _numFragments);
  }
}

void ShapeFragment::CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center, float& _radius)
{
  Matrix34 totalMatrix = m_transform * _transform;
  LegacyVector3 center = totalMatrix * m_center;

  float distance = (center - _center).Mag();
  _radius = std::max(distance + m_radius, _radius);

  int numFragments = m_childFragments.Size();
  for (int i = 0; i < numFragments; ++i)
  {
    ShapeFragment* frag = m_childFragments.GetData(i);
    frag->CalculateRadius(totalMatrix, _center, _radius);
  }
}

Shape::Shape() {}

Shape::Shape(const char* filename, bool _animating)
  : m_animating(_animating),
    m_displayListName(nullptr),
    m_rootFragment(nullptr)
{
  TextFileReader in(filename);
  Load(&in);
}

Shape::Shape(TextReader* in, bool _animating)
  : m_animating(_animating),
    m_displayListName(nullptr),
    m_rootFragment(nullptr)
{
  Load(in);
}

Shape::~Shape()
{
  delete m_rootFragment;
}

void Shape::Load(TextReader* _in)
{
  m_name = _in->GetFilename();

  constexpr int maxFrags = 100;
  constexpr int maxMarkers = 100;
  int currentFrag = 0;
  int currentMarker = 0;
  ShapeFragment* allFrags[maxFrags];
  ShapeMarker* allMarkers[maxMarkers];

  while (_in->ReadLine())
  {
    if (!_in->TokenAvailable())
      continue;

    char* c = _in->GetNextToken();

    if (_stricmp(c, "fragment") == 0)
    {
      DEBUG_ASSERT(currentFrag < maxFrags);
      c = _in->GetNextToken();
      allFrags[currentFrag] = NEW ShapeFragment(_in, c);
      currentFrag++;
    }
    else if (_stricmp(c, "marker") == 0)
    {
      DEBUG_ASSERT(currentMarker < maxMarkers);
      c = _in->GetNextToken();
      allMarkers[currentMarker] = NEW ShapeMarker(_in, c);
      currentMarker++;
    }
  }

  m_rootFragment = NEW ShapeFragment("SceneRoot", "");

  // We need to build the hierarchy of fragments from the flat array
  for (int i = 0; i < currentFrag; ++i)
  {
    if (_stricmp(allFrags[i]->m_parentName.c_str(), "SceneRoot") == 0) { m_rootFragment->m_childFragments.PutData(allFrags[i]); }
    else
    {
      // find the ith fragment's parent
      int j;
      for (j = 0; j < currentFrag; ++j)
      {
        if (i == j)
          continue;
        DEBUG_ASSERT(_stricmp(allFrags[i]->m_name.c_str(), allFrags[j]->m_name.c_str()) != 0);
        if (_stricmp(allFrags[i]->m_parentName.c_str(), allFrags[j]->m_name.c_str()) == 0)
        {
          allFrags[j]->m_childFragments.PutData(allFrags[i]);
          break;
        }
      }
      DEBUG_ASSERT(j < currentFrag);
    }
  }

  // Add the ShapeMarkers into the fragment tree
  for (int i = 0; i < currentMarker; ++i)
  {
    ShapeFragment* parent = m_rootFragment->LookupFragment(allMarkers[i]->m_parentName.c_str());
    DEBUG_ASSERT(parent);
    parent->m_childMarkers.PutData(allMarkers[i]);

    int depth = allMarkers[i]->m_depth - 1;
    allMarkers[i]->m_parents[depth] = parent;
    depth--;
    while (_stricmp(parent->m_name.c_str(), "SceneRoot") != 0)
    {
      parent = m_rootFragment->LookupFragment(parent->m_parentName.c_str());
      DEBUG_ASSERT(parent && depth >= 0);
      allMarkers[i]->m_parents[depth] = parent;
      depth--;
    }
    DEBUG_ASSERT(depth == -1);
  }
}

void Shape::Render(float _predictionTime, const Matrix34& _transform)
{
  glEnable(GL_COLOR_MATERIAL);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(_transform.ConvertToOpenGLFormat());

  m_rootFragment->Render(_predictionTime);

  glDisable(GL_COLOR_MATERIAL);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

bool Shape::RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate)
{
  bool rv = m_rootFragment->RayHit(_package, _transform, _accurate);
  return rv;
}

bool Shape::SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate)
{
  bool hit = m_rootFragment->SphereHit(_package, _transform, _accurate);
  return hit;
}

bool Shape::ShapeHit(Shape* _shape, const Matrix34& _theTransform, const Matrix34& _ourTransform, bool _accurate)
{
  bool hit = m_rootFragment->ShapeHit(_shape, _theTransform, _ourTransform, _accurate);
  return hit;
}

LegacyVector3 Shape::CalculateCenter(const Matrix34& _transform)
{
  LegacyVector3 center;
  int numFragments = 0;

  m_rootFragment->CalculateCenter(_transform, center, numFragments);

  center /= static_cast<float>(numFragments);

  return center;
}

float Shape::CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center)
{
  float radius = 0.0f;

  m_rootFragment->CalculateRadius(_transform, _center, radius);

  return radius;
}
