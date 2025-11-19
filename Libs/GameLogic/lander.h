#ifndef _included_lander_h
#define _included_lander_h

#include "entity.h"

class Lander : public Entity
{
  protected:
    enum
    {
      StateSailing,
      StateLanded
    };

    int m_state;
    float m_spawnTimer;

  public:
    Lander();

    bool Advance(Unit* _unit) override;
    bool AdvanceSailing();
    bool AdvanceLanded();

    void ChangeHealth(int amount) override;
    void Render(float _predictionTime, int _teamId);
};

#endif
