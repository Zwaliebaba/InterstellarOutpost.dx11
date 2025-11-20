#pragma once

#include "building.h"

class Cave : public Building
{
  public:
    Cave();

    bool Advance() override;
    void Damage(float _damage) override;

  protected:
    EntityType m_troopType;
    int m_unitId;
    float m_spawnTimer;
    bool m_dead;

    ShapeMarker* m_spawnPoint;
};
