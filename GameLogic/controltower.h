#pragma once

#include "building.h"

class ControlTower : public Building
{
  public:
    ControlTower();

    void Initialize(Building* _template) override;

    bool Advance() override;

    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    int GetAvailablePosition(LegacyVector3& _pos, LegacyVector3& _front);         // Finds place for reprogrammer
    void GetConsolePosition(int _position, LegacyVector3& _pos);

    void BeginReprogram(int _position);
    bool Reprogram(int _teamId);                            // Returns true if job completed
    void EndReprogram(int _position);

    void Read(TextReader* _in, bool _dynamic) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

  public:
    float m_ownership;                            // 100 = strongly owned, 5 = nearly lost, 0 = neutral

  protected:
    static Shape* s_dishShape;
    ShapeMarker* m_lightPos;
    ShapeMarker* m_reprogrammer[3];
    ShapeMarker* m_console[3];
    ShapeMarker* m_dishPos;

    Matrix34 m_dishMatrix;

    bool m_beingReprogrammed[3];                 // One bool for each slot
    int m_controlBuildingId;                    // Whom I affect

    float m_checkTargetTimer;
};
