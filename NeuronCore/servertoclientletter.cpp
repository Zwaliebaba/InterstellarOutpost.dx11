#include "pch.h"
#include "ServerToClientLetter.h"

ServerToClientLetter::ServerToClientLetter()
  : m_type(Invalid),
    m_teamId(0),
    m_teamType(0),
    m_ip(0),
    m_clientId(-1),
    m_sequenceId(0) {}

ServerToClientLetter::ServerToClientLetter(DataReader& _dataReader)
  : m_type(Invalid),
    m_teamId(0),
    m_teamType(0),
    m_ip(0),
    m_clientId(-1),
    m_sequenceId(0)
{
  m_type = (LetterType)_dataReader.Read<int16_t>();
  m_sequenceId = _dataReader.Read<int32_t>();

  switch (m_type)
  {
    case HelloClient:
    case GoodbyeClient:
      m_ip = _dataReader.Read<int32_t>();
      break;

    case TeamAssign:
      m_teamId = _dataReader.Read<uint8_t>(); 
      m_teamType = _dataReader.Read<uint8_t>();
      m_ip = _dataReader.Read<int32_t>();
      break;

    case Update:
      int numUpdates = _dataReader.Read<int32_t>();
      DEBUG_ASSERT(numUpdates >= 0);

      for (int i = 0; i < numUpdates; ++i)
      {
        m_updates.emplace_back(NetworkUpdate(_dataReader));
      }
      break;
  }
}

void ServerToClientLetter::AddUpdate(const NetworkUpdate& _update)
{
  m_updates.emplace_back(_update);
}

void ServerToClientLetter::WriteByteStream(DataWriter& _dataWriter)
{
  _dataWriter.WriteUInt16(m_type);
  _dataWriter.WriteUInt32(m_sequenceId);

  switch (m_type)
  {
    case HelloClient:
    case GoodbyeClient:
      _dataWriter.WriteUInt32(m_ip);
      break;

    case TeamAssign:
      _dataWriter.WriteByte(m_teamId);
      _dataWriter.WriteByte(m_teamType);
      _dataWriter.WriteUInt32(m_ip);
      break;

    case Update:
      int numUpdates = m_updates.size();
      DEBUG_ASSERT(numUpdates >= 0);
      _dataWriter.WriteUInt32(numUpdates);

      for (int i = 0; i < numUpdates; ++i)
      {
        m_updates[i].WriteByteStream(_dataWriter);
      }
      break;
  }
}
