#include "pch.h"
#include "Server.h"
#include "ClientPeer.h"
#include "NetLib.h"
#include "NetSocket.h"
#include "NetworkConst.h"
#include "Packet.h"
#include "ServerToClientLetter.h"
#include "globals.h"
#include "preferences.h"
#include "profiler.h"

namespace
{
  void ListenCallback(const Packet& _udpPacket)
  {
    const auto fromAddr = _udpPacket.GetClientAddress();
    char newip[16];
    IpToString(fromAddr->sin_addr, newip);

      DataReader reader(_udpPacket.GetBuffer(), _udpPacket.GetSize());
      NetworkUpdate letter(reader);
      Server::ReceiveLetter(letter, newip);
  }
}

ServerTeam::ServerTeam(const int _clientId)
  : m_clientId(_clientId) {}

void Server::Startup()
{
  m_socket = NEW NetSocket();

  Windows::System::Threading::ThreadPool::RunAsync([](auto&&) { m_socket->StartListening(ListenCallback, SERVER_PORT); });
}

void Server::Shutdown() {}

int Server::GetClientId(const char* _ip)
{
  for (int i = 0; i < m_clients.Size(); ++i)
  {
    if (m_clients.ValidIndex(i))
    {
      char* thisIP = m_clients[i]->GetIP();
      if (strcmp(thisIP, _ip) == 0)
        return i;
    }
  }

  return -1;
}

int Server::ConvertIPToInt(const char* _ip)
{
  ASSERT_TEXT(strlen(_ip) < 17, "IP address too long");
  char ipCopy[17];
  strcpy(ipCopy, _ip);
  size_t ipLen = strlen(ipCopy);

  for (size_t i = 0; i < ipLen; ++i)
  {
    if (ipCopy[i] == '.')
      ipCopy[i] = '\n';
  }

  int part1, part2, part3, part4;
  sscanf(ipCopy, "%d %d %d %d", &part1, &part2, &part3, &part4);

  int result = ((part4 & 0xff) << 24) + ((part3 & 0xff) << 16) + ((part2 & 0xff) << 8) + (part1 & 0xff);
  return result;
}

char* Server::ConvertIntToIP(const int _ip)
{
  static char result[16];
  sprintf(result, "%d.%d.%d.%d", (_ip & 0x000000ff), (_ip & 0x0000ff00) >> 8, (_ip & 0x00ff0000) >> 16, (_ip & 0xff000000) >> 24);

  return result;
}

// ***RegisterNewClient
void Server::RegisterNewClient(const char* _ip)
{
  DEBUG_ASSERT(GetClientId(_ip) == -1);
  auto sToC = new ClientPeer(_ip);
  m_clients.PutData(sToC);

  //
  // Tell all clients about it

  ServerToClientLetter letter;
  letter.SetType(ServerToClientLetter::HelloClient);
  letter.SetIp(ConvertIPToInt(_ip));
  SendLetter(letter);
}

void Server::RemoveClient(const char* _ip)
{
  int clientId = GetClientId(_ip);
  ClientPeer* sToC = m_clients[clientId];
  m_clients.MarkNotUsed(clientId);
  delete sToC;

  //
  // Tell all clients about it

  ServerToClientLetter letter;
  letter.SetType(ServerToClientLetter::GoodbyeClient);
  letter.SetIp(ConvertIPToInt(_ip));
  SendLetter(letter);
}

// *** RegisterNewTeam
void Server::RegisterNewTeam(char* _ip, int _teamType, int _desiredTeamId)
{
  int clientId = GetClientId(_ip);
  DEBUG_ASSERT(clientId != -1);                           // Client not properly connected

  if (_desiredTeamId != -1)                              // Specified Team ID - An AI
  {
    if (!m_teams.ValidIndex(_desiredTeamId))
    {
      auto team = new ServerTeam(clientId);
      if (m_teams.Size() <= _desiredTeamId)
        m_teams.SetSize(_desiredTeamId + 1);
      m_teams.PutData(team, _desiredTeamId);
    }
  }
  else
  {
    DEBUG_ASSERT(m_teams.NumUsed() < NUM_TEAMS);
    auto team = new ServerTeam(clientId);
    int teamId = m_teams.PutData(team);

    ServerToClientLetter letter;
    letter.SetType(ServerToClientLetter::TeamAssign);
    letter.SetTeamId(teamId);
    letter.SetIp(ConvertIPToInt(_ip));
    letter.SetTeamType(_teamType);
    SendLetter(letter);
  }
}

bool Server::GetNextLetter(NetworkUpdate& _update) { return m_inbox.try_pop(_update); }

void Server::ReceiveLetter(NetworkUpdate& update, char* fromIP)
{
  update.SetClientIp(fromIP);
  m_inbox.push(update);
}

void Server::SendLetter(ServerToClientLetter& letter)
{
  // Assign a sequence id
  letter.SetSequenceId(m_sequenceId);
  m_sequenceId++;

  m_history.emplace_back(letter);
}

void Server::AdvanceSender()
{
  ServerToClientLetter letter;

  while (m_outbox.try_pop(letter))
  {
    if (m_clients.ValidIndex(letter.GetClientId()))
    {
      ClientPeer* client = m_clients[letter.GetClientId()];
      DataWriter memStream;
      letter.WriteByteStream(memStream);
      m_socket->SendUDPPacket(client->GetHost(), memStream);
    }
  }
}

void Server::Advance()
{
  StartProfile(L"Advance Server");

  //
  // Compile all incoming messages into a ServerToClientLetter

  ServerToClientLetter letter;
  letter.SetType(ServerToClientLetter::Update);

  NetworkUpdate incoming;

  while (GetNextLetter(incoming))
  {
    if (incoming.m_type == NetworkUpdate::ClientJoin)
    {
      if (GetClientId(incoming.m_clientIp) == -1)
      {
        DebugTrace("SERVER: New Client connected from {}\n", incoming.m_clientIp);
        RegisterNewClient(incoming.m_clientIp);
      }
    }
    else if (incoming.m_type == NetworkUpdate::ClientLeave)
    {
      if (GetClientId(incoming.m_clientIp) != -1)
      {
        DebugTrace("SERVER: Client at {} disconnected gracefully\n", incoming.m_clientIp);
        RemoveClient(incoming.m_clientIp);
      }
    }
    else if (incoming.m_type == NetworkUpdate::RequestTeam)
    {
      if (GetClientId(incoming.m_clientIp) != -1)
      {
        DebugTrace("SERVER: New team request from {}\n", incoming.m_clientIp);
        RegisterNewTeam(incoming.m_clientIp, incoming.m_teamType, incoming.m_desiredTeamId);
      }
    }
    else if (incoming.m_type == NetworkUpdate::Syncronise)
    {
      int sequenceId = incoming.m_lastProcessedSeqId;
      unsigned char sync = incoming.m_sync;

      if (sequenceId != 0 && !m_sync.ValidIndex(sequenceId - 1))
      {
        // This incoming packet has a sequence ID that is too high
        // Most likely it was sent from a previous client connected to a previous server
        // Then that server shut down, and this one started up
        // Then this server received the packet intended for the old server
        // So we simply discard it
        //DebugTrace( "Sync %d discarded as bogus\n", sequenceId );
      }
      else
      {
        if (m_sync.Size() <= sequenceId)
          m_sync.SetSize(m_sync.Size() + 1000);

        if (m_sync.ValidIndex(sequenceId))
        {
          unsigned char lastKnownSync = m_sync[sequenceId];
          DEBUG_ASSERT(lastKnownSync == sync);
        }
        else
          m_sync.PutData(sync, sequenceId);
      }
    }
    else if (incoming.m_teamId != 255)
      letter.AddUpdate(incoming);

    int clientId = GetClientId(incoming.m_clientIp);
    if (clientId != -1)
    {
      ClientPeer* sToc = m_clients[clientId];
      sToc->m_lastKnownSequenceId = std::max(incoming.m_lastSequenceId, sToc->m_lastKnownSequenceId);
    }
  }

  SendLetter(letter);

  //
  // Update all clients depending on their state

  int maxUpdates = 25;                                            // Sensible to cap re-transmissions like this

  for (int i = 0; i < m_clients.Size(); ++i)
  {
    if (m_clients.ValidIndex(i))
    {
      ClientPeer* s2c = m_clients[i];
      int sendFrom = s2c->m_lastKnownSequenceId + 1;
      int sendTo = m_history.size();
      if (sendTo - sendFrom > maxUpdates)
        sendTo = sendFrom + maxUpdates;

      for (int l = sendFrom; l < sendTo; ++l)
      {
        ServerToClientLetter& theLetter = m_history[l];
        ServerToClientLetter letterCopy(theLetter);
        letterCopy.SetClientId(i);

        m_outbox.push(letterCopy);
      }
    }
  }

  AdvanceSender();

  EndProfile();
}
