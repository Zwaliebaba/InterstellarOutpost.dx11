#pragma once

#include "ClientPeer.h"
#include "NetworkUpdate.h"
#include "ServerToClientLetter.h"

class ServerTeam
{
  public:
    int m_clientId;

    ServerTeam(int _clientId);
};

class Server
{
  public:
    static void Startup();
    static void Shutdown();

    static bool GetNextLetter(NetworkUpdate& _update);

    static void ReceiveLetter(NetworkUpdate& update, char* fromIP);
    static void SendLetter(ServerToClientLetter& letter);

    static int GetClientId(const char* _ip);
    static void RegisterNewClient(const char* _ip);
    static void RemoveClient(const char* _ip);
    static void RegisterNewTeam(char* _ip, int _teamType, int _desiredTeamId);

    static void AdvanceSender();
    static void Advance();

    static int ConvertIPToInt(const char* _ip);
    static char* ConvertIntToIP(int _ip);

    inline static int m_sequenceId = 0;

    inline static DArray<ClientPeer*> m_clients;
    inline static DArray<ServerTeam*> m_teams;

    inline static Concurrency::concurrent_queue<NetworkUpdate> m_inbox;
    inline static Concurrency::concurrent_queue<ServerToClientLetter> m_outbox;

    inline static DArray<unsigned char> m_sync;                                     // Synchronisation values for each sequenceId

  protected:
    inline static NetSocket* m_socket;
    inline static std::vector<ServerToClientLetter> m_history;
};
