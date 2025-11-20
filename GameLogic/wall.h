#ifndef _included_wall_h
#define _included_wall_h

#include "building.h"

class Wall : public Building
{
  protected:
    float m_damage;
    float m_fallSpeed;

  public:
    Wall();

    bool Advance() override;
    void Damage(float _damage) override;
    void Render(float _predictionTime) override;
};

#endif
