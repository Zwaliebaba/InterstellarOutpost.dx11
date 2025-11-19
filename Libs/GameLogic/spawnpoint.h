#ifndef _included_spawnpoint_h
#define _included_spawnpoint_h

#include "building.h"

// ============================================================================

class SpawnBuildingSpirit
{
  public:
    int m_targetBuildingId;
    float m_currentProgress;
};

class SpawnBuildingLink
{
  public:
    int m_targetBuildingId;                         // Who I am directly linked to

    LList<int> m_targets;                           // List of all SpawnPoint buildings reachable down this route
    LList<SpawnBuildingSpirit*> m_spirits;
};

class SpawnBuilding : public Building
{
  protected:
    LList<SpawnBuildingLink*> m_links;
    ShapeMarker* m_spiritLink;

    LegacyVector3 m_visibilityMidpoint;
    float m_visibilityRadius;

  public:
    SpawnBuilding(BuildingType _type);
    ~SpawnBuilding() override;

    void Initialize(Building* _template) override;
    bool Advance() override;

    virtual void TriggerSpirit(SpawnBuildingSpirit* _spirit);

    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;
    void RenderSpirit(const LegacyVector3& _pos);

    LegacyVector3 GetSpiritLink();
    void SetBuildingLink(int _buildingId) override;
    void ClearLinks();

    LList<int>* ExploreLinks();                     // Returns a list of all SpawnBuildings accessable

    void Read(TextReader* _in, bool _dynamic) override;
    
};

// ============================================================================

class SpawnLink : public SpawnBuilding
{
  public:
    SpawnLink();
};

// ============================================================================

class MasterSpawnPoint : public SpawnBuilding
{
  protected:
    static int s_masterSpawnPointId;
    bool m_exploreLinks;

  public:
    MasterSpawnPoint();

    bool Advance() override;
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    void RequestSpirit(int _targetBuildingId);

    const char* GetObjectiveCounter() override;

    static MasterSpawnPoint* GetMasterSpawnPoint();
};

// ============================================================================

class SpawnPoint : public SpawnBuilding
{
  protected:
    void RecalculateOwnership();
    bool PopulationLocked();

    float m_evaluateTimer;
    float m_spawnTimer;
    int m_populationLock;               // Building ID (if found), -1 = not yet searched, -2 = nothing found
    int m_numFriendsNearby;
    ShapeMarker* m_doorMarker;

  public:
    SpawnPoint();

    bool Advance() override;

    bool PerformDepthSort(LegacyVector3& _centerPos) override;

    void TriggerSpirit(SpawnBuildingSpirit* _spirit) override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;
    void RenderPorts() override;
};

// ============================================================================

class SpawnPopulationLock : public Building
{
  public:
    float m_searchRadius;
    int m_maxPopulation;
    int m_teamCount[NUM_TEAMS];

  protected:
    static float s_overpopulationTimer;
    static int s_overpopulation;
    int m_originalMaxPopulation;
    float m_recountTimer;
    int m_recountTeamId;

  public:
    SpawnPopulationLock();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
};

#endif
