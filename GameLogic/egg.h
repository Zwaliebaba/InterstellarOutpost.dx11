#pragma once

#include "entity.h"

#define EGG_DORMANTLIFE     120                             // total time alive while unfertilised

class Egg : public Entity
{
  public:
    enum
    {
      StateDormant,
      StateFertilised,
      StateOpen
    };

    int m_state;
    int m_spiritId;
    float m_timer;

    Egg();

    void ChangeHealth(int amount) override;

    void Render(float predictionTime) override;
    bool Advance(Unit* _unit) override;
    void Fertilise(int spiritId);
};
