#ifndef SERVER_TO_CLIENT_LETTER_H
#define SERVER_TO_CLIENT_LETTER_H

#include "NetworkUpdate.h"

class ServerToClientLetter
{
  public:
    enum LetterType
    {
      Invalid,
      HelloClient,
      GoodbyeClient,
      TeamAssign,
      Update
    };

    ServerToClientLetter();
    ServerToClientLetter(DataReader& _dataReader);

    // Copy / move support (const correctness required for concurrent_queue)
    ServerToClientLetter& operator=(const ServerToClientLetter& rhs) = default;

    void SetClientId(int _id) { m_clientId = _id; }
    void SetType(LetterType _type) { m_type = _type; }
    void SetSequenceId(int seqId) { m_sequenceId = seqId; }
    void SetTeamId(int teamId) { m_teamId = teamId; }
    void SetTeamType(int teamType) { m_teamType = teamType; }
    void SetIp(int ip) { m_ip = ip; }

    int GetClientId() const { return m_clientId; }
    int GetSequenceId() const { return m_sequenceId; }

    void AddUpdate(const NetworkUpdate& _update);

    void WriteByteStream(DataWriter& _dataWriter);

    LetterType m_type;                      // If you add any new data here, remember to update the copy constructor
    unsigned char m_teamId;
    unsigned char m_teamType;
    int m_ip;                               // This tells you specifically which client gets the HelloClient or TeamAssign

    std::vector<NetworkUpdate> m_updates;

  private:
    int m_clientId;                 // An index into the server's DArray of ClientPeer objects
    int m_sequenceId;
};

#endif
