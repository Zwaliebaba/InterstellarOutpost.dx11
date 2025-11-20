#ifndef _included_staticshape_h
#define _included_staticshape_h

#include "building.h"

class StaticShape : public Building
{
  public:
    char m_shapeName[256];
    float m_scale;

    StaticShape();

    void Initialize(Building* _template) override;

    void SetShapeName(const char* _shapeName);
    void SetStringId(char* _stringId);

    bool Advance() override;
    void Render(float _predictionTime) override;

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10,
                    LegacyVector3* _pos = nullptr,
                    LegacyVector3* _norm = nullptr) override;        // pos/norm will not always be available

    void Read(TextReader* _in, bool _dynamic) override;
    
};

#endif
