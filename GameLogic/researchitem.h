#ifndef _included_researchitem_h
#define _included_researchitem_h

#include "building.h"

class ResearchItem : public Building
{
  protected:
    float m_reprogrammed;
    ShapeMarker* m_end1;
    ShapeMarker* m_end2;

  public:
    int m_researchType;                             // indexes into GlobalResearch::m_type
    int m_level;
    bool m_inLibrary;

    ResearchItem();

    void Initialize(Building* _template) override;

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool NeedsReprogram();
    bool Reprogram();

    void Read(TextReader* _in, bool _dynamic) override;
    

    void GetEndPositions(LegacyVector3& _end1, LegacyVector3& _end2);

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen, LegacyVector3* _pos,
                    LegacyVector3* norm) override;

    bool IsInView() override;
};

#endif
