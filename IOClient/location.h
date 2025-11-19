#ifndef INCLUDED_LOCATION_H
#define INCLUDED_LOCATION_H

#include "LegacyVector3.h"
#include "building.h"
#include "entity.h"
#include "entity_grid.h"
#include "fast_darray.h"
#include "landscape.h"
#include "level_file.h"
#include "obstruction_grid.h"
#include "slice_darray.h"
#include "spirit.h"
#include "team.h"
#include "weapons.h"
#include "worldobject.h"

#undef GetObject

class Location
{
  protected:
    int m_lastSliceProcessed;
    bool m_missionComplete;

    void LoadLevel(std::string_view _missionFilename, std::string_view _mapFilename);

    void AdvanceWeapons(int _slice);
    void AdvanceBuildings(int _slice);
    void AdvanceTeams(int _slice);
    void AdvanceSpirits(int _slice);

    void RenderLandscape();
    void RenderWeapons();
    void RenderBuildings();
    void RenderBuildingAlphas();
    void RenderParticles();
    void RenderTeams();
    void RenderSpirits();

    void InitLandscape();
    void InitLights();
    void InitTeams();

    void DoMissionCompleteActions();

    LegacyVector3 FindValidSpawnPosition(const LegacyVector3& _pos, float _spread);

  public:
    Landscape m_landscape;
    EntityGrid* m_entityGrid;
    ObstructionGrid* m_obstructionGrid;
    LevelFile* m_levelFile;

    Team* m_teams;

    float m_christmasTimer;

    FastDArray<Light*> m_lights;
    SliceDArray<Building*> m_buildings;
    SliceDArray<Spirit> m_spirits;
    SliceDArray<Laser> m_lasers;
    SliceDArray<Effect*> m_effects;
    std::vector<std::shared_ptr<WorldObject>> m_globalWorldObjects;

    Location();
    ~Location();

    void Init(std::string_view _missionFilename, std::string_view _mapFilename);
    void InitBuildings();
    void Empty();

    void Advance(int _slice);
    void Render();

    void InitialiseTeam(unsigned char _teamId, unsigned char _teamType);

    void RemoveTeam(unsigned char _teamId);

    std::optional<std::shared_ptr<WorldObject>> GetWorldObject(const LegacyVector3& _startRay, const LegacyVector3& _rayDir,
                                                               unsigned char _teamId, float _maxDistance = FLT_MAX,
                                                               float* _range = nullptr) const;

    int GetBuildingId(const LegacyVector3& startRay, const LegacyVector3& direction, unsigned char teamId,
                      float _maxDistance = FLT_MAX, float* _range = nullptr);
    int GetUnitId(const LegacyVector3& startRay, const LegacyVector3& direction, unsigned char teamId, float* _range = nullptr);
    WorldObjectId GetEntityId(const LegacyVector3& startRay, const LegacyVector3& direction, unsigned char teamId,
                              float* _range = nullptr);

    bool IsWalkable(const LegacyVector3& _from, const LegacyVector3& _to, bool _evaluateCliffs = false);
    bool IsVisible(const LegacyVector3& _from, const LegacyVector3& _to);

    void UpdateTeam(unsigned char teamId, const TeamControls& teamControls);

    int SpawnSpirit(const LegacyVector3& _pos, const LegacyVector3& _vel, unsigned char _teamId, WorldObjectId _id);
    void ThrowWeapon(const LegacyVector3& _pos, const LegacyVector3& _target, EffectType _type, unsigned char _fromTeamId);
    void FireRocket(const LegacyVector3& _pos, const LegacyVector3& _target, unsigned char _fromTeamId);
    void FireLaser(const LegacyVector3& _pos, const LegacyVector3& _vel, unsigned char _fromTeamId);
    void FireTurretShell(const LegacyVector3& _pos, const LegacyVector3& _vel);
    void Bang(const LegacyVector3& _pos, float _range, float _damage);
    void CreateShockwave(const LegacyVector3& _pos, float _size, unsigned char _teamId = 255);

    bool MissionComplete();

    void AdvanceChristmas();
    static int ChristmasModEnabled();           // 0 = unavailable, 1 = enabled, 2 = disabled

    WorldObjectId SpawnEntities(const LegacyVector3& _pos, unsigned char _teamId, int _unitId, EntityType _type, int _numEntities,
                                const LegacyVector3& _vel, float _spread, float _range = -1.0f, int _routeId = -1,
                                int _routeWaypointId = -1);

    int GetSpirit(WorldObjectId _id);

    bool IsFriend(unsigned char _teamId1, unsigned char _teamId2);

    Team* GetMyTeam();
    Entity* GetEntity(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir);
    Building* GetBuilding(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir);

    WorldObject* GetWorldObject(WorldObjectId _id);
    Entity* GetEntity(WorldObjectId _id);
    Entity* GetEntitySafe(WorldObjectId _id, EntityType _type);                 // Safe to cast
    Unit* GetUnit(WorldObjectId _id);
    WorldObject* GetEffect(WorldObjectId _id);
    Building* GetBuilding(int _id);
    Spirit* GetSpirit(int _index);

    void SetupFog();
    void SetupLights();

    void RegenerateOpenGlState();
};

#endif
