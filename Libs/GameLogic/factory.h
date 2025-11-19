#pragma once

#include "building.h"
#include "entity.h"
#include "spiritstore.h"

class Factory : public Building
{
  public:
    EntityType m_troopType;
    unsigned char m_stats[Entity::NumStats];

    int m_initialCapacity;		// Read from level file

    int m_unitId;
    int m_numToCreate;
    int m_numCreated;

    float m_timeToCreate;         // Total Time to create ALL troops
    float m_timeSoFar;

    enum
    {
      StateUnused,
      StateCreating,
      StateRecharging
    };

    int m_state;

    SpiritStore m_spiritStore;

    Factory();

    void Initialize(Building* _template) override;

    void Render(float predictionTime) override;
    void RenderAlphas(float predictionTime) override;

    bool Advance() override;
    void AdvanceStateUnused();
    void AdvanceStateCreating();
    void AdvanceStateRecharging();

    void SetTeamId(int _teamId) override;

    void RequestUnit(EntityType _troopType, int _numToCreate);

    void Read(TextReader* _in, bool _dynamic) override;
};