#pragma once

#include "building.h"

#define ANTHILL_SEARCHRANGE     400.0f

struct AntObjective
{
  LegacyVector3 m_pos;
  WorldObjectId m_targetId;
  int m_numToSend;
};

class AntHill : public Building
{
  protected:
    LList<AntObjective*> m_objectives;

    float m_objectiveTimer;
    float m_spawnTimer;
    float m_eggConvertTimer;
    int m_health;
    int m_unitId;
    int m_populationLock;
    bool m_renderDamaged;

    bool SearchingArea(LegacyVector3 _pos);
    bool TargettedEntity(WorldObjectId _id);

    bool SearchForSpirits(LegacyVector3& _pos);
    bool SearchForDarwinians(LegacyVector3& _pos, WorldObjectId& _id);
    bool SearchForEnemies(LegacyVector3& _pos, WorldObjectId& _id);

    bool SearchForScoutArea(LegacyVector3& _pos);

    bool PopulationLocked();

  public:
    int m_numAntsInside;
    int m_numSpiritsInside;

    AntHill();

    void Initialize(Building* _template) override;

    bool Advance() override;
    void Render(float _predictionTime) override;
    void Damage(float _damage) override;
    void Destroy(float _intensity) override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};
