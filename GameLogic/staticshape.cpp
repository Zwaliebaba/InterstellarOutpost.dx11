#include "pch.h"
#include "staticshape.h"
#include "GameApp.h"
#include "location.h"
#include "resource.h"
#include "shape.h"
#include "text_stream_readers.h"

StaticShape::StaticShape()
  : Building(BuildingType::TypeStaticShape),
    m_scale(1.0f)
{
  strcpy(m_shapeName, "none");
}

void StaticShape::Initialize(Building* _template)
{
  Building::Initialize(_template);

  auto staticShape = dynamic_cast<StaticShape*>(_template);

  m_scale = staticShape->m_scale;
  SetShapeName(staticShape->m_shapeName);

  m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

  if (m_shape)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    mat.u *= m_scale;
    mat.r *= m_scale;
    mat.f *= m_scale;

    m_centerPos = m_shape->CalculateCenter(mat);
    m_radius = m_shape->CalculateRadius(mat, m_centerPos);
  }
}

void StaticShape::SetShapeName(const char* _shapeName)
{
  strcpy(m_shapeName, _shapeName);

  if (strcmp(m_shapeName, "none") != 0)
  {
    SetShape(Resource::GetShape(m_shapeName));

    Matrix34 mat(m_front, m_up, m_pos);
    mat.u *= m_scale;
    mat.r *= m_scale;
    mat.f *= m_scale;

    m_centerPos = m_shape->CalculateCenter(mat);
    m_radius = m_shape->CalculateRadius(mat, m_centerPos);
  }
}

bool StaticShape::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                             LegacyVector3* norm)
{
  if (m_shape)
  {
    RayPackage ray(_rayStart, _rayDir, _rayLen);
    Matrix34 transform(m_front, m_up, m_pos);
    transform.u *= m_scale;
    transform.r *= m_scale;
    transform.f *= m_scale;
    return m_shape->RayHit(&ray, transform, true);
  }
  return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
}

bool StaticShape::DoesSphereHit(const LegacyVector3& _pos, float _radius)
{
  if (m_shape)
  {
    SpherePackage sphere(_pos, _radius);
    Matrix34 transform(m_front, m_up, m_pos);
    transform.u *= m_scale;
    transform.r *= m_scale;
    transform.f *= m_scale;
    return m_shape->SphereHit(&sphere, transform);
  }
  float distance = (_pos - m_pos).Mag();
  return (distance <= _radius + m_radius);
}

bool StaticShape::DoesShapeHit(Shape* _shape, Matrix34 _theTransform)
{
  if (m_shape)
  {
    Matrix34 ourTransform(m_front, m_up, m_pos);
    ourTransform.u *= m_scale;
    ourTransform.r *= m_scale;
    ourTransform.f *= m_scale;

    return _shape->ShapeHit(m_shape, ourTransform, _theTransform, true);
  }
  SpherePackage package(m_pos, m_radius);
  return _shape->SphereHit(&package, _theTransform, true);
}

void StaticShape::Render(float _predictionTime)
{
  if (m_shape)
  {
    Matrix34 mat(m_front, m_up, m_pos);
    mat.u *= m_scale;
    mat.r *= m_scale;
    mat.f *= m_scale;

    glEnable(GL_NORMALIZE);
    m_shape->Render(_predictionTime, mat);
    glDisable(GL_NORMALIZE);
  }
}

bool StaticShape::Advance() { return Building::Advance(); }

void StaticShape::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_scale = atof(_in->GetNextToken());

  SetShapeName(_in->GetNextToken());
}
