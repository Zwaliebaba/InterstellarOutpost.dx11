#pragma once

#include "DataReader.h"
#include "DataWriter.h"
#include "team.h"

class NetworkUpdate
{
  public:
    enum UpdateType
    {
      Invalid,                            // Unused
      ClientJoin,                         // New Client connects
      ClientLeave,                        // Client disconnects
      RequestTeam,                        // Client requests a new team, either virtual player or real
      Alive,                              // Updates Avatar position
      SelectUnit,                         // A player selected a unit
      CreateUnit,                         // A player issued instructions to a factory
      AimBuilding,                        // A player is aiming something like a radar dish
      ToggleLaserFence,                   // A laser fence is toggled
      RunProgram,                         // A player wishes to run a program
      TargetProgram,                      // A player targets a running program
      Pause,
      Syncronise                          // Performs a check to make sure we're in sync
    };

    UpdateType m_type;
    char m_clientIp[16];
    unsigned char m_teamType;
    signed char m_desiredTeamId;		// Used by the server host to force AI players to have a specific ID. -1 if we don't care
    int m_unitId;
    int m_entityId;
    int m_buildingId;
    EntityType m_entityType;
    int m_numTroops;
    unsigned char m_teamId;
    LegacyVector3 m_direction;
    unsigned char m_program;
    TeamControls m_teamControls;

    int32_t m_lastProcessedSeqId;       // Used for sync checks
    unsigned char m_sync;

    int32_t m_lastSequenceId;

    NetworkUpdate();
    NetworkUpdate(DataReader& _dataReader);

    void SetType(UpdateType _type);
    void SetClientIp(char* ip);
    void SetTeamType(unsigned char _teamType);
    void SetDesiredTeamId(signed char _desiredTeamId);
    void SetWorldPos(const LegacyVector3& _pos);
    void SetTeamControls(const TeamControls& _teamControls);
    void SetTeamId(unsigned char _teamId);
    void SetEntityType(EntityType _type);
    void SetNumTroops(int _numTroops);
    void SetDirection(LegacyVector3 _dir);
    void SetUnitId(int _unitId);
    void SetEntityId(int _entityId);
    void SetBuildingID(int _buildingId);
    void SetProgram(unsigned char _prog);
    void SetLastProcessedId(int _lastProcessedId);
    void SetSync(unsigned char _sync);

    void SetLastSequenceId(int _lastSequenceId);

    const LegacyVector3& GetWorldPos() const;
    LegacyVector3& GetWorldPos();

    void ReadByteStream(DataReader& _dataReader);                          // Returns number of bytes read
    void WriteByteStream(DataWriter& _dataWriter);
};

inline const LegacyVector3& NetworkUpdate::GetWorldPos() const { return m_teamControls.m_mousePos; }

inline LegacyVector3& NetworkUpdate::GetWorldPos() { return m_teamControls.m_mousePos; }
