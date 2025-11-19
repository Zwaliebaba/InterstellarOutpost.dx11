#pragma once

#include "unit.h"

class AirstrikeUnit : public Unit
{
  public:
    LegacyVector3 m_enterPosition;
    LegacyVector3 m_attackPosition;
    LegacyVector3 m_exitPosition;

    LegacyVector3 m_front;                            // Current direction
    LegacyVector3 m_up;
    float m_speed;

    int m_effectId;
    int m_numInvaders;

    enum
    {
      StateApproaching,
      StateLeaving
    };

    int m_state;

    bool AdvanceToTargetPosition(LegacyVector3 _targetPos);

    AirstrikeUnit(int teamId, int unitId, int numEntities, const LegacyVector3& _pos);

    void Begin() override;
    bool Advance(int _slice) override;
    void Render(float _predictionTime) override;

    bool IsInView() override;
};

class SpaceInvader : public Entity
{
  protected:
    LegacyVector3 m_targetPos;
    bool m_armed;
    Shape* m_bombShape;

  public:
    SpaceInvader();

    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;
    void Render(float _predictionTime) override;
};
