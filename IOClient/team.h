#pragma once

#include "entity.h"
#include "fast_darray.h"
#include "rgb_colour.h"
#include "slice_darray.h"
#include "worldobject.h"

class Unit;
class InsertionSquad;

class Team
{
  public:
    enum
    {
      TeamTypeUnused = -1,
      TeamTypeLocalPlayer,
      TeamTypeRemotePlayer,
      TeamTypeCPU
    };

    int m_teamId;
    int m_teamType;

    FastDArray<Unit*> m_units;
    SliceDArray<Entity*> m_others;
    LList<WorldObjectId> m_specials;             // Officers and tanks for quick lookup

    RGBAColor m_colour;

    int m_currentUnitId;                    //
    int m_currentEntityId;                  // Do not set these directly
    int m_currentBuildingId;                // They are updated by the network
    //
    LegacyVector3 m_currentMousePos;                  //

    Team();

    void Initialize(int _teamId);                  // Call when this team enters the game
    void SetTeamType(int _teamType);

    void SelectUnit(int _unitId, int _entityId, int _buildingId);

    void RegisterSpecial(WorldObjectId _id);
    void UnRegisterSpecial(WorldObjectId _id);

    Entity* RayHitEntity(const LegacyVector3& _rayStart, const LegacyVector3& _rayEnd);
    Unit* GetMyUnit();
    Entity* GetMyEntity();
    Unit* NewUnit(EntityType _troopType, int _numEntities, int* _unitId, const LegacyVector3& _pos);
    Entity* NewEntity(EntityType _troopType, int _unitId, int* _index);

    int NumEntities(EntityType _troopType);               // Counts the total number

    void Advance(int _slice);

    void Render();
    void RenderVirii(float _predictionTime);
    void RenderDarwinians(float _predictionTime);
    void RenderOthers(float _predictionTime);
};

// ****************************************************************************
//  Class TeamControls
//
//   capture all the control information necessary to send
//   over the network for "remote" control of units
// ****************************************************************************

class TeamControls
{
  public:
    TeamControls();

    unsigned short GetFlags() const;
    void SetFlags(unsigned short _flags);
    void ClearFlags();
    void Advance();
    void Clear();

    LegacyVector3 m_mousePos;

    // Be sure to update GetFlags, SetFlags, ZeroFlags if you change these flags
    // Also, NetworkUpdate::WriteByteStream and NetworkUpdate::ReadByteStream
    // if you use more than 8 bits

    unsigned int m_unitMove              : 1;
    unsigned int m_primaryFireTarget     : 1;
    unsigned int m_secondaryFireTarget   : 1;
    unsigned int m_primaryFireDirected   : 1;
    unsigned int m_secondaryFireDirected : 1;
    unsigned int m_cameraEntityTracking  : 1;
    unsigned int m_directUnitMove        : 1;
    unsigned int m_unitSecondaryMode     : 1;
    unsigned int m_endSetTarget          : 1;

    int m_directUnitMoveDx;
    int m_directUnitMoveDy;
    int m_directUnitFireDx;
    int m_directUnitFireDy;
};

