#ifndef INCLUDED_LEVEL_FILE
#define INCLUDED_LEVEL_FILE

#include "entity.h"
#include "landscape.h"
#include "llist.h"
#include "text_stream_readers.h"
#include "worldobject.h"

#define MAGIC_MOUNT_NAME_START_POS	"CamPosBefore"

class Building;
class Route;
class GlobalEventCondition;

class CameraMount
{
  public:
    std::string m_name;
    LegacyVector3 m_pos;
    LegacyVector3 m_front;
    LegacyVector3 m_up;
};

enum class TransitionMode : int8_t
{
  Unknown = -1,
  TransitionMove,
  TransitionCut
};
ENUM_HELPER(TransitionMode, TransitionMove, TransitionCut);

class CamAnimNode
{
  public:
    CamAnimNode() = default;
    ~CamAnimNode() = default;

    static TransitionMode GetTransitModeId(const char* _word);
    static std::string GetTransitModeName(TransitionMode _modeId);

    TransitionMode m_transitionMode = {TransitionMode::TransitionMove};
    std::string m_mountName;
    float m_duration = {1.0f};
};

class CameraAnimation
{
  public:
    CameraAnimation() = default;
    ~CameraAnimation() = default;

    std::string m_name;
    std::vector<CamAnimNode> m_nodes;
};

class InstantUnit
{
  public:
    InstantUnit()
      : m_entityType(EntityType::Invalid),
        m_teamId(-1),
        m_posX(0.0f),
        m_posZ(0.0f),
        m_number(0),
        m_inAUnit(false),
        m_state(-1),
        m_spread(200.0f),
        m_waypointX(0.0f),
        m_waypointZ(0.0f),
        m_routeId(-1),
        m_routeWaypointId(-1) {}

    EntityType m_entityType;
    int m_teamId;
    float m_posX;
    float m_posZ;
    int m_number;
    bool m_inAUnit;
    int m_state;
    float m_spread;
    float m_waypointX;
    float m_waypointZ;
    int m_routeId;
    int m_routeWaypointId;
};

class LandscapeFlattenArea
{
  public:
    LegacyVector3 m_center;
    float m_size;
};

class LandscapeDef
{
  public:
    LList<LandscapeTile*> m_tiles;
    LList<LandscapeFlattenArea*> m_flattenAreas;
    float m_cellSize;
    int m_worldSizeX;
    int m_worldSizeZ;
    float m_outsideHeight;

    LandscapeDef()
      : m_cellSize(12.0f),
        m_worldSizeX(2000),
        m_worldSizeZ(2000),
        m_outsideHeight(-10) {}

    ~LandscapeDef()
    {
      m_tiles.EmptyAndDelete();
      m_flattenAreas.EmptyAndDelete();
    }
};

class RunningProgram
{
  public:
    EntityType m_entityType;
    int m_count;
    int m_state;
    int m_data;
    float m_waypointX;
    float m_waypointZ;

    float m_positionX[10];
    float m_positionZ[10];
    int m_health[10];
};

class LevelFile
{
  public:
    LevelFile();
    LevelFile(std::string_view _missionFilename, std::string_view _mapFilename);
    ~LevelFile();

    Building* GetBuilding(int _id);
    CameraMount* GetCameraMount(std::string_view _name);
    int GetCameraAnimId(std::string_view _name);
    void RemoveBuilding(int _id);
    Route* GetRoute(int _id);

    std::string m_missionFilename;
    std::string m_mapFilename;

    std::string m_landscapeColorFilename;

    LList<CameraMount*> m_cameraMounts;
    LList<CameraAnimation*> m_cameraAnimations;
    LList<Building*> m_buildings;
    LList<InstantUnit*> m_instantUnits;
    LList<Light*> m_lights;
    LList<Route*> m_routes;
    LList<RunningProgram*> m_runningPrograms;
    LList<GlobalEventCondition*> m_primaryObjectives;
    LList<GlobalEventCondition*> m_secondaryObjectives;
    int m_levelDifficulty;

    LandscapeDef m_landscape;

  protected:
    void ParseMissionFile(std::string_view _filename);
    void ParseMapFile(std::string_view _filename);

    void ParseCameraMounts(TextReader* _in);
    void ParseCameraAnims(TextReader* _in);
    void ParseBuildings(TextReader* _in, bool _dynamic);
    void ParseInstantUnits(TextReader* _in);
    void ParseLandscapeData(TextReader* _in);
    void ParseLandscapeTiles(TextReader* _in);
    void ParseLandFlattenAreas(TextReader* _in);
    void ParseLights(TextReader* _in);
    void ParseRoute(TextReader* _in, int _id);
    void ParseRoutes(TextReader* _in);
    void ParsePrimaryObjectives(TextReader* _in);
    void ParseRunningPrograms(TextReader* _in);
    void ParseDifficulty(TextReader* _in);

    void GenerateAutomaticObjectives();
};

#endif
