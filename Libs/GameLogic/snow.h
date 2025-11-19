#pragma once

#include "Effect.h"

class Snow : public Effect
{
  protected:
    float m_timeSync;
    float m_positionOffset;                       // Used to make them float around a bit
    float m_xaxisRate;
    float m_yaxisRate;
    float m_zaxisRate;

  public:
    LegacyVector3 m_hover;

    Snow();

    bool Advance() override;
    void Render(float _predictionTime) override;

    float GetLife();                        // Returns 0.0f-1.0f (0.0f=dead, 1.0f=alive)
};
