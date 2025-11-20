#pragma once

#include "building.h"

class FeedingTube : public Building
{
  protected:
    ShapeMarker* m_focusMarker;

    int m_receiverId;
    float m_range;
    float m_signal;

    LegacyVector3 GetDishPos(float _predictionTime);      // Returns the position of the transmission point
    LegacyVector3 GetDishFront(float _predictionTime);      // Returns the front vector of the dish
    LegacyVector3
    GetForwardsClippingDir(float _predictionTime, FeedingTube* _sender);// Returns a good clipping direction for signal

    void RenderSignal(float _predictionTime, float _radius, float _alpha);

  public:
    FeedingTube();

    void Initialize(Building* _template) override;
    void Read(TextReader* _in, bool _dynamic) override;

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    LegacyVector3 GetStartPoint();
    LegacyVector3 GetEndPoint();

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;

    bool IsInView() override;
};
