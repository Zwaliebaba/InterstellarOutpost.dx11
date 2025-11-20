#include "pch.h"
#include "NetworkUpdate.h"
#include "DataReader.h"

NetworkUpdate::NetworkUpdate()
  : m_type(Invalid),
    m_clientIp{},
    m_teamType(0),
    m_desiredTeamId(0),
    m_unitId(0),
    m_entityId(0),
    m_buildingId(-1),
    m_entityType(EntityType::Invalid),
    m_numTroops(0),
    m_teamId(255),
    m_program(0),
    m_lastProcessedSeqId(0),
    m_sync(0),
    m_lastSequenceId(-1) {}

NetworkUpdate::NetworkUpdate(DataReader& _dataReader) { ReadByteStream(_dataReader); }

void NetworkUpdate::ReadByteStream(DataReader& _dataReader)
{
  m_type = static_cast<UpdateType>(_dataReader.Read<int16_t>());
  m_lastSequenceId = _dataReader.Read<int32_t>();

  switch (m_type)
  {
    case ClientJoin:
    case ClientLeave:
      break;

    case RequestTeam:
    {
      m_teamType = _dataReader.Read<uint8_t>();
      m_entityType = static_cast<EntityType>(_dataReader.Read<uint8_t>());
      m_desiredTeamId = _dataReader.Read<uint8_t>();
    }
    break;

    case Alive:
    {
      m_teamId = _dataReader.Read<uint8_t>();
      GetWorldPos().x = _dataReader.Read<float>();
      GetWorldPos().y = _dataReader.Read<float>();
      GetWorldPos().z = _dataReader.Read<float>();
      auto flags = _dataReader.Read<uint16_t>();
      m_teamControls.SetFlags(flags);
      m_sync = _dataReader.Read<uint8_t>();
    }
    break;

    case Syncronise:
    {
      m_lastProcessedSeqId = _dataReader.Read<int32_t>();
      m_sync = _dataReader.Read<uint8_t>();
    }
    break;

    case SelectUnit:
    {
      m_teamId = _dataReader.Read<uint8_t>();
      m_unitId = _dataReader.Read<int32_t>();
      m_entityId = _dataReader.Read<int32_t>();
      m_buildingId = _dataReader.Read<int32_t>();
    }
    break;

    case CreateUnit:
    {
      m_teamId = _dataReader.Read<uint8_t>();
      m_entityType = static_cast<EntityType>(_dataReader.Read<uint8_t>());
      m_numTroops = _dataReader.Read<int32_t>();
      m_buildingId = _dataReader.Read<int32_t>();
      GetWorldPos().x = _dataReader.Read<float>();
      GetWorldPos().y = _dataReader.Read<float>();
      GetWorldPos().z = _dataReader.Read<float>();
    }
    break;

    case AimBuilding:
    {
      m_teamId = _dataReader.Read<uint8_t>();
      m_buildingId = _dataReader.Read<int32_t>();
      GetWorldPos().x = _dataReader.Read<float>();
      GetWorldPos().y = _dataReader.Read<float>();
      GetWorldPos().z = _dataReader.Read<float>();
    }
    break;

    case ToggleLaserFence:
      m_buildingId = _dataReader.Read<int32_t>();
      break;

    case RunProgram:
      m_teamId = _dataReader.Read<uint8_t>();
      m_program = _dataReader.Read<uint8_t>();
      break;

    case TargetProgram:
    {
      m_teamId = _dataReader.Read<uint8_t>();
      m_program = _dataReader.Read<uint8_t>();
      GetWorldPos().x = _dataReader.Read<float>();
      GetWorldPos().y = _dataReader.Read<float>();
      GetWorldPos().z = _dataReader.Read<float>();
    }
    break;

    case Invalid: DEBUG_ASSERT(false);
  }
}

void NetworkUpdate::SetType(UpdateType _type) { m_type = _type; }

void NetworkUpdate::SetClientIp(char* ip) { strcpy(m_clientIp, ip); }

void NetworkUpdate::SetTeamType(unsigned char _teamType) { m_teamType = _teamType; }

void NetworkUpdate::SetDesiredTeamId(signed char _desiredTeamId) { m_desiredTeamId = _desiredTeamId; }

void NetworkUpdate::SetWorldPos(const LegacyVector3& _pos)
{
  // Shared with m_teamControls
  GetWorldPos() = _pos;
}

void NetworkUpdate::SetTeamControls(const TeamControls& _teamControls) { m_teamControls = _teamControls; }

void NetworkUpdate::SetTeamId(unsigned char _teamId) { m_teamId = _teamId; }

void NetworkUpdate::SetEntityType(EntityType _type) { m_entityType = _type; }

void NetworkUpdate::SetNumTroops(int _numTroops) { m_numTroops = _numTroops; }

void NetworkUpdate::SetDirection(LegacyVector3 _dir) { m_direction = _dir; }

void NetworkUpdate::SetLastSequenceId(int _lastSequenceId) { m_lastSequenceId = _lastSequenceId; }

void NetworkUpdate::SetUnitId(int _unitId) { m_unitId = _unitId; }

void NetworkUpdate::SetEntityId(int _entityId) { m_entityId = _entityId; }

void NetworkUpdate::SetBuildingID(int _buildingId) { m_buildingId = _buildingId; }

void NetworkUpdate::SetProgram(unsigned char _prog) { m_program = _prog; }

void NetworkUpdate::SetLastProcessedId(int _lastProcessedId) { m_lastProcessedSeqId = _lastProcessedId; }

void NetworkUpdate::SetSync(unsigned char _sync) { m_sync = _sync; }

void NetworkUpdate::WriteByteStream(DataWriter& _dataWriter)
{
  _dataWriter.WriteInt16(m_type);
  _dataWriter.WriteInt32(m_lastSequenceId);

  switch (m_type)
  {
    case ClientJoin:
    case ClientLeave:
      break;

    case RequestTeam:
      _dataWriter.WriteByte(m_teamType);
      _dataWriter.WriteByte(I(m_entityType));
      _dataWriter.WriteByte(m_desiredTeamId);
      break;

    case Alive:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteFloat(GetWorldPos().x);
      _dataWriter.WriteFloat(GetWorldPos().y);
      _dataWriter.WriteFloat(GetWorldPos().z);
      _dataWriter.WriteUInt16(m_teamControls.GetFlags());
      _dataWriter.WriteByte(m_sync);
      break;

    case Syncronise:
      _dataWriter.WriteUInt32(m_lastProcessedSeqId);
      _dataWriter.WriteByte(m_sync);
      break;

    case SelectUnit:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteUInt32(m_unitId);
      _dataWriter.WriteUInt32(m_entityId);
      _dataWriter.WriteUInt32(m_buildingId);
      break;

    case CreateUnit:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteByte(I(m_entityType));
      _dataWriter.WriteUInt32(m_numTroops);
      _dataWriter.WriteUInt32(m_buildingId);
      _dataWriter.WriteFloat(GetWorldPos().x);
      _dataWriter.WriteFloat(GetWorldPos().y);
      _dataWriter.WriteFloat(GetWorldPos().z);
      break;

    case AimBuilding:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteUInt32(m_buildingId);
      _dataWriter.WriteFloat(GetWorldPos().x);
      _dataWriter.WriteFloat(GetWorldPos().y);
      _dataWriter.WriteFloat(GetWorldPos().z);
      break;

    case ToggleLaserFence:
      _dataWriter.WriteUInt32(m_buildingId);
      break;

    case RunProgram:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteByte(m_program);
      break;

    case TargetProgram:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteByte(m_program);
      _dataWriter.WriteFloat(GetWorldPos().x);
      _dataWriter.WriteFloat(GetWorldPos().y);
      _dataWriter.WriteFloat(GetWorldPos().z);
      break;

    case Invalid: DEBUG_ASSERT(false);
  }
}
