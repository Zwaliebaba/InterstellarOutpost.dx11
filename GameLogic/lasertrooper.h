#ifndef _included_lasertrooper_h
#define _included_lasertrooper_h

#include "entity.h"

class LaserTrooper : public Entity
{
  public:
    LaserTrooper()
      : Entity(EntityType::TypeLaserTroop) {}

    LegacyVector3 m_targetPos;
    LegacyVector3 m_unitTargetPos;

    float m_victoryDance = 0.0f;

    void Begin() override;

    bool Advance(Unit* _unit) override;
    void Render(float _predictionTime, int _teamId);

    void AdvanceVictoryDance();
};

#endif
