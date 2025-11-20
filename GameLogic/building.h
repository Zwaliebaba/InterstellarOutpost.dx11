#pragma once

#include "LegacyVector3.h"
#include "globals.h"
#include "llist.h"
#include "matrix34.h"
#include "shape.h"
#include "text_stream_readers.h"
#include "worldobject.h"

class BuildingPort;

enum class BuildingType : int8_t
{
  TypeInvalid = -1,		// When you add an entry here remember to update building.cpp
  TypeFactory,
  TypeCave,
  TypeRadarDish,
  TypeLaserFence,
  TypeControlTower,
  TypeGunTurret,
  TypeBridge,
  TypePowerstation,
  TypeWall,
  TypeTrunkPort,
  TypeResearchItem,
  TypeLibrary,
  TypeGenerator,
  TypePylon,
  TypePylonStart,
  TypePylonEnd,
  TypeSolarPanel,
  TypeTrackLink,
  TypeTrackJunction,
  TypeTrackStart,
  TypeTrackEnd,
  TypeRefinery,
  TypeMine,
  TypeYard,
  TypeDisplayScreen,
  TypeUpgradePort,
  TypePrimaryUpgradePort,
  TypeIncubator,
  TypeAntHill,
  TypeSafeArea,
  TypeTriffid,
  TypeSpiritReceiver,
  TypeReceiverLink,
  TypeReceiverSpiritSpawner,
  TypeSpiritProcessor,
  TypeSpawnPoint,
  TypeSpawnPopulationLock,
  TypeSpawnPointMaster,
  TypeSpawnLink,
  TypeAITarget,
  TypeAISpawnPoint,
  TypeBlueprintStore,
  TypeBlueprintConsole,
  TypeBlueprintRelay,
  TypeScriptTrigger,
  TypeSpam,
  TypeGodDish,
  TypeStaticShape,
  TypeFuelGenerator,
  TypeFuelPipe,
  TypeFuelStation,
  TypeEscapeRocket,
  TypeFenceSwitch,
  TypeDynamicHub,
  TypeDynamicNode,
  TypeFeedingTube
};
ENUM_HELPER(BuildingType, TypeFactory, TypeFeedingTube);

class Building : public WorldObject
{
  public:
    Building(BuildingType _type);

    virtual void Initialize(Building* _template);
    bool Advance() override;

    virtual void SetShape(Shape* _shape);
    void SetShapeLights(ShapeFragment* _fragment);   // Recursivly search for lights
    void SetShapePorts(ShapeFragment* _fragment);

    virtual bool IsInView();

    void Render(float predictionTime) override;
    virtual void RenderAlphas(float predictionTime);
    virtual void RenderLights();
    virtual void RenderPorts();

    virtual bool PerformDepthSort(LegacyVector3& _centerPos);       // Return true if you plan to use transparencies

    virtual void SetTeamId(int _teamId);
    virtual void Reprogram(float _complete);
    virtual void ReprogramComplete();

    virtual void Damage(float _damage);
    virtual void Destroy(float _intensity);

    LegacyVector3 PushFromBuilding(const LegacyVector3& _pos, float _radius);

    virtual void EvaluatePorts();
    virtual int GetNumPorts();
    virtual int GetNumPortsOccupied();
    virtual WorldObjectId GetPortOccupant(int _portId);
    virtual bool GetPortPosition(int _portId, LegacyVector3& _pos, LegacyVector3& _front);

    virtual void OperatePort(int _portId, int _teamId);
    virtual int GetPortOperatorCount(int _portId, int _teamId);

    virtual const char* GetObjectiveCounter();

    virtual bool DoesSphereHit(const LegacyVector3& _pos, float _radius);
    virtual bool DoesShapeHit(Shape* _shape, Matrix34 _transform);
    virtual bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10,
                            LegacyVector3* _pos = nullptr,
                            LegacyVector3* _norm = nullptr);        // pos/norm will not always be available

    virtual void Read(TextReader* _in, bool _dynamic);     // Use these to read/write additional building-specific

    virtual int GetBuildingLink();                             // Allows a building to link to another
    virtual void SetBuildingLink(int _buildingId);            // eg control towers

    static const char* GetTypeName(BuildingType _type);
    static BuildingType GetTypeId(const char* _name);
    static Building* CreateBuilding(BuildingType _type);
    static Building* CreateBuilding(const char* _name);

    static std::string GetTypeNameTranslated(BuildingType _type);

    BuildingType m_buildingType;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float m_timeOfDeath;
    bool m_dynamic;								// Only appears on this level, not all levels for this map
    bool m_isGlobal;

    bool m_destroyed;							// Building has been destroyed using the script command DestroyBuilding, remove it next Advance

    Shape* m_shape;
    LList<ShapeMarker*> m_lights;               // Ownership lights
    LList<BuildingPort*> m_ports;               // Require Darwinians in them to operate

    static Shape* s_controlPad;
    static ShapeMarker* s_controlPadStatus;
};

class BuildingPort
{
  public:
    ShapeMarker* m_marker;
    WorldObjectId m_occupant;
    Matrix34 m_mat;
    int m_counter[NUM_TEAMS];
};
