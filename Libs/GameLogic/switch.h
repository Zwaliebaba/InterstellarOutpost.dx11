#ifndef _included_fenceswitch_h
#define _included_fenceswitch_h

#include "building.h"

class Shape;
class ShapeFragment;
class ShapeMarker;

class FenceSwitch : public Building
{
  protected:
    int m_linkedBuildingId;
    int m_linkedBuildingId2;    // optional second link for fence toggling

    bool m_switchable;

    float m_timer;                // no fences changes will be made until this timer counts to 0 for the first time

    ShapeMarker* m_connectionLocation;

  public:
    char m_script[256];
    bool m_locked;
    int m_lockable;
    int m_switchValue;

    FenceSwitch();

    void Initialize(Building* _template) override;

    bool Advance() override;
    void Render(float predictionTime) override;
    void RenderAlphas(float predictionTime) override;
    void RenderLights() override;
    void RenderConnection(LegacyVector3 _targetPos, bool _active);

    void Switch();

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    void Read(TextReader* _in, bool _dynamic) override;

    LegacyVector3 GetConnectionLocation();

    bool IsInView() override;
};

#endif
