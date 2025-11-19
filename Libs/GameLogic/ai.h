#pragma once

#include "location.h"
#include "entity.h"

class AI : public Entity
{
  protected:
    int FindTargetBuilding(int _fromTargetId, int _fromTeamId);
    int FindNearestTarget(const LegacyVector3& _fromPos);

    float m_timer;

  public:
    AI();

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;
};

// ============================================================================

#define AITARGET_LINKRANGE      600.0f

class AITarget : public Building
{
  protected:
    float m_teamCountTimer;

  public:
    LList<int> m_neighbours;                               // Building IDs of nearby AITargets

    int m_friendCount[NUM_TEAMS];
    int m_enemyCount[NUM_TEAMS];
    int m_idleCount[NUM_TEAMS];
    float m_priority[NUM_TEAMS];

    AITarget();

    bool Advance() override;
    void RenderAlphas(float _predictionTime) override;

    void RecalculateNeighbours();
    void RecountTeams();
    void RecalculateOwnership();
    void RecalculatePriority();

    float IsNearTo(int _aiTargetId);                            // returns distance or -1

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
};

// ============================================================================

class AISpawnPoint : public Building
{
  protected:
    float m_timer;                // Master timer between spawns
    bool m_online;
    int m_numSpawned;           // Number spawned this batch
    int m_populationLock;       // Building ID (if found), -1 = not yet searched, -2 = nothing found

    bool PopulationLocked();

  public:
    EntityType m_entityType;
    int m_count;
    int m_period;
    int m_activatorId;          // Building ID
    int m_spawnLimit;           // limits the number of times this building can spawn
    int m_routeId;				// Route path that spawned units should follow

    AISpawnPoint();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
};
