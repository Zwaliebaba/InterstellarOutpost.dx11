#ifndef _included_laserfence_h
#define _included_laserfence_h

#include "building.h"

class Shape;
class ShapeFragment;

#define LASERFENCE_RAISESPEED       0.3f

class LaserFence : public Building
{
  protected:
    float m_status;                       // 0=down, 1=up
    int m_nextLaserFenceId;
    float m_sparkTimer;

    bool m_radiusSet;

    ShapeMarker* m_marker1;
    ShapeMarker* m_marker2;

    bool
    m_nextToggled;		// set to true when the fence has enabled/disabled the next fence in the line, to prevent constant enable calling

  public:
    enum
    {
      ModeDisabled,
      ModeEnabling,
      ModeEnabled,
      ModeDisabling,
      ModeNeverOn
    };

    int m_mode;
    float m_scale;

    LaserFence();

    void Initialize(Building* _template) override;

    bool Advance() override;
    void Render(float predictionTime) override;
    void RenderAlphas(float predictionTime) override;
    void RenderLights() override;

    bool PerformDepthSort(LegacyVector3& _centerPos) override;
    bool IsInView() override;

    void Read(TextReader* _in, bool _dynamic) override;
    

    void Enable();
    void Disable();
    void Toggle();
    bool IsEnabled();

    void Spark();
    void Electrocute(const LegacyVector3& _pos);

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    float GetFenceFullHeight();

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10,
                    LegacyVector3* _pos = nullptr, LegacyVector3* _norm = nullptr) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;

    LegacyVector3 GetTopPosition();
};

#endif
