#ifndef _included_safearea_h
#define _included_safearea_h

#include "building.h"
#include "entity.h"

class SafeArea : public Building
{
  public:
    float m_size;
    int m_entitiesRequired;
    EntityType m_entityTypeRequired;

    float m_recountTimer;
    int m_entitiesCounted;

    SafeArea();

    void Initialize(Building* _template) override;
    bool Advance() override;

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10,
                    LegacyVector3* _pos = nullptr, LegacyVector3* _norm = nullptr) override;

    const char* GetObjectiveCounter() override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

#endif
