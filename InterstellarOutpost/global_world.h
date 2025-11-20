#ifndef _included_global_world_h
#define _included_global_world_h

#include "building.h"
#include "llist.h"
#include "matrix34.h"
#include "text_stream_readers.h"

class LegacyVector3;
class Shape;
class Building;
class GlobalInternet;

class GlobalLocation
{
  public:
    int m_id;
    LegacyVector3 m_pos;
    bool m_available;            // Is it connected on the transit system

    std::string m_name;
    std::string m_mapFilename;
    std::string m_missionFilename;
    bool m_missionCompleted;

    int m_numSpirits;           // Number of spirits that have died

    GlobalLocation();

    void AddSpirits(int _count = 1);
};

class GlobalBuilding
{
  public:
    int m_id;
    int m_teamId;
    int m_locationId;
    LegacyVector3 m_pos;
    BuildingType m_buildingType;
    bool m_online;
    int m_link;
    Shape* m_shape;

    GlobalBuilding();
};

class GlobalEventCondition
{
  public:
    enum
    {
      AlwaysTrue,			// 0
      BuildingOnline,		// 1
      BuildingOffline,	// 2
      ResearchOwned,		// 3
      NotInLocation,		// 4
      DebugKey,			// 5
      NeverTrue,			// 6
      NumConditions                                       // Remember to update GetTypeName
    };

    GlobalEventCondition();
    ~GlobalEventCondition();

    bool Evaluate();

    void SetStringId(const char* _stringId);
    void SetCutScene(const char* _cutScene);

    static const char* GetTypeName(int _type);
    static int GetType(const char* _typeName);

    int m_type;
    int m_id;
    int m_locationId;
    std::string m_stringId;                                    // Brief description
    std::string m_cutScene;                                    // Filename of cutscene to run
};

class GlobalEventAction
{
  public:
    enum
    {
      SetMission,
      RunScript,
      MakeAvailable,
      NumActionTypes
    };

    GlobalEventAction();

    void Read(TextReader* _in);
    void Execute();

    static const char* GetTypeName(int _type);

    int m_type;
    int m_locationId;
    std::string m_filename;
};

class GlobalEvent
{
  public:
    GlobalEvent();
    GlobalEvent(GlobalEvent& _other);	// Copy constructor only used by TestHarness

    void Read(TextReader* _in);
    bool Evaluate();
    bool Execute();                                 // Returns true when all done

    void MakeAlwaysTrue();

    LList<GlobalEventCondition*> m_conditions;
    LList<GlobalEventAction*> m_actions;
};

#define GLOBALRESEARCH_TIMEPERPOINT             10
#define GLOBALRESEARCH_POINTS_CONTROLTOWER      22

class GlobalResearch
{
  public:
    enum
    {
      TypeDarwinian,
      TypeOfficer,
      TypeSquad,
      TypeLaser,
      TypeGrenade,
      TypeRocket,
      TypeController,
      TypeAirStrike,
      TypeArmour,
      TypeTaskManager,
      TypeEngineer,
      NumResearchItems
    };

    GlobalResearch();

    bool HasResearch(int _type);
    int CurrentProgress(int _type);
    int CurrentLevel(int _type);

    void AddResearch(int _type);
    void SetCurrentProgress(int _type, int _progress);

    void IncreaseProgress(int _amount);
    void DecreaseProgress(int _amount);
    int RequiredProgress(int _level);             // Progress required to reach this level

    void EvaluateLevel(int _type);

    void SetCurrentResearch(int _type);
    void GiveResearchPoints(int _numPoints);
    void AdvanceResearch();

    void Read(TextReader* _in);

    static std::string GetTypeName(int _type);
    static int GetType(const char* _name);

    static std::string GetTypeNameTranslated(int _type);

    int m_researchLevel[NumResearchItems];
    int m_researchProgress[NumResearchItems];
    int m_currentResearch;
    int m_researchPoints;
    float m_researchTimer;
};

class SphereWorld
{
  public:
    SphereWorld();

    void AddLocation(int _locationId);

    void Render();
    void RenderWorldShape();
    void RenderIslands();
    void RenderTrunkLinks();
    void RenderHeaven();
    void RenderSpirits();

    Shape* m_shapeOuter;
    Shape* m_shapeMiddle;
    Shape* m_shapeInner;

    int m_numLocations;
    LList<float>* m_spirits;    // An array with one LList<float> per location
};

class GlobalWorld
{
  public:
    GlobalWorld();
    GlobalWorld(GlobalWorld&); // Copy constructor only used in TestHarness
    ~GlobalWorld();

    void Advance();
    void Render();

    int LocationHit(const LegacyVector3& _pos, const LegacyVector3& _dir, float locationRadius = 5000.0f);

    void AddLocation(GlobalLocation* _location);
    void AddBuilding(GlobalBuilding* _building);

    GlobalLocation* GetLocation(int _id);
    GlobalLocation* GetLocation(std::string_view _name);
    GlobalLocation* GetHighlightedLocation();                         // ie whats under the mouse
    int GetLocationId(std::string_view _name);
    int GetLocationIdFromMapFilename(std::string_view _mapFilename);
    std::string GetLocationName(int _id);
    std::string GetLocationNameTranslated(int _id);
    LegacyVector3 GetLocationPosition(int _id);

    GlobalBuilding* GetBuilding(int _id, int _locationId);
    int GenerateBuildingId();

    bool EvaluateEvents();	                        // Returns true if an event was triggered
    void TransferSpirits(int _locationId);

    void LoadGame(std::string_view _filename);

    void LoadLocations(std::string_view _filename);

    void SetupLights();
    void SetupFog();

    float GetSize();

  public:
    GlobalInternet* m_globalInternet;
    SphereWorld* m_sphereWorld;
    GlobalResearch* m_research;

    LList<GlobalLocation*> m_locations;
    LList<GlobalBuilding*> m_buildings;
    LList<GlobalEvent*> m_events;
    int m_myTeamId;

    int m_editorMode;
    int m_editorSelectionId;

  protected:
    void ParseLocations(TextReader* _in);
    void ParseBuildings(TextReader* _in);
    void ParseEvents(TextReader* _in);

    void AddLevelBuildingToGlobalBuildings(Building* _building, int _locId);

    int m_nextLocationId;
    int m_nextBuildingId;

    int m_locationRequested;	// Stores the location a user has clicked on while we fade out. -1 means no request yet.
};

#endif
