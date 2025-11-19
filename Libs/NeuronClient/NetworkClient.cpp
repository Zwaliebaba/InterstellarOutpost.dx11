#include "pch.h"
#include "NetworkClient.h"
#include "GameApp.h"
#include "NetLib.h"
#include "NetSocket.h"
#include "NetworkConst.h"
#include "Packet.h"
#include "Server.h"
#include "ServerToClientLetter.h"
#include "factory.h"
#include "hi_res_time.h"
#include "input.h"
#include "laserfence.h"
#include "location.h"
#include "main.h"
#include "preferences.h"
#include "radardish.h"
#include "taskmanager.h"
#include "team.h"

namespace
{
  void ListenCallback(const Packet& _udpPacket)
  {
    const auto fromAddr = _udpPacket.GetClientAddress();
    char newip[16];
    IpToString(fromAddr->sin_addr, newip);

    DataReader reader(_udpPacket.GetBuffer(), _udpPacket.GetSize());
    auto letter = NEW ServerToClientLetter(reader);
    g_app->m_networkClient->ReceiveLetter(letter);
  }
}

NetworkClient::NetworkClient()
{
  m_lastValidSequenceIdFromServer = -1;

  m_netLib = NEW NetLib();
  m_netLib->Startup();

  m_socket = NEW NetSocket();

  auto server = g_prefsManager->GetString("ServerAddress");

  m_serverHost = {};
  m_serverHost.sin_family = AF_INET;
  m_serverHost.sin_port = htons(SERVER_PORT);

  HOSTENT* pHostent = gethostbyname(server);
  if (!pHostent)
    DebugTrace("Host address resolution failed for {}", server);
  m_serverHost.sin_addr.s_addr = *(unsigned long*)pHostent->h_addr_list[0];

  Windows::System::Threading::ThreadPool::RunAsync([&](auto&&)
  {
    NetRetCode retCode = m_socket->StartListening(ListenCallback, CLIENT_PORT);
    if (retCode != NetOk)
    {
      int err = WSAGetLastError();
      char buf[200];
      buf[0] = '\0';
      FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);

      DebugTrace("Failed to start listening with error message \"{}\"", buf);
    }
    DEBUG_ASSERT(retCode == NetOk);
  });
}

NetworkClient::~NetworkClient()
{
  while (m_outbox.Size() > 0) {}

  m_inbox.EmptyAndDelete();
  m_outbox.EmptyAndDelete();
  SAFE_DELETE(m_netLib);
  SAFE_DELETE(m_socket);
}

void NetworkClient::AdvanceSender()
{
  m_outboxMutex.lock();

  while (m_outbox.Size())
  {
    NetworkUpdate* letter = m_outbox[0];
    DEBUG_ASSERT(letter);

    DataWriter byteStream;
    letter->WriteByteStream(byteStream);
    m_socket->SendUDPPacket(m_serverHost, byteStream);
    delete letter;

    m_outbox.RemoveData(0);
  }
  m_outboxMutex.unlock();
}

void NetworkClient::Advance() { AdvanceSender(); }

int NetworkClient::GetOurIP_Int()
{
  // We're not doing networking for now
  static int s_localIP = Server::ConvertIPToInt("127.0.0.1");
  return s_localIP;
}

int NetworkClient::GetNextLetterSeqID()
{
  int result = -1;

  m_inboxMutex.lock();
  if (m_inbox.Size() > 0)
    result = m_inbox[0]->GetSequenceId();
  m_inboxMutex.unlock();

  return result;
}

ServerToClientLetter* NetworkClient::GetNextLetter()
{
  m_inboxMutex.lock();
  ServerToClientLetter* letter = nullptr;

  if (m_inbox.Size() > 0)
  {
    letter = m_inbox[0];
    if (letter->GetSequenceId() == g_lastProcessedSequenceId + 1)
      m_inbox.RemoveData(0);
    else
      letter = nullptr;
  }

  m_inboxMutex.unlock();

  return letter;
}

void NetworkClient::ReceiveLetter(ServerToClientLetter* letter)
{
  //
  // Simulate network packet loss

#ifdef _DEBUG
  if (g_inputManager->controlEvent(ControlDebugDropPacket))
  {
    delete letter;
    return;
  }
#endif

  //
  // Check for duplicates

  if (letter->GetSequenceId() <= m_lastValidSequenceIdFromServer)
  {
    delete letter;
    return;
  }

  //
  // Work out our start time

  double newStartTime = GetHighResTime() - static_cast<float>(letter->GetSequenceId()) * SERVER_ADVANCE_PERIOD;
  if (newStartTime < g_startTime)
    g_startTime = newStartTime;
    //#ifdef _DEBUG
  else if (newStartTime > g_startTime + 0.1f)
  {
    g_startTime = newStartTime;
    //        DebugTrace( "Start Time set to %f\n", (float) g_startTime );
  }
  //#endif

  //
  // Do a sorted insert of the letter into the inbox

  m_inboxMutex.lock();
  int i;
  bool inserted = false;
  for (i = m_inbox.Size() - 1; i >= 0; --i)
  {
    ServerToClientLetter* thisLetter = m_inbox[i];
    if (letter->GetSequenceId() > thisLetter->GetSequenceId())
    {
      m_inbox.PutDataAtIndex(letter, i + 1);
      inserted = true;
      break;
    }
    if (letter->GetSequenceId() == thisLetter->GetSequenceId())
    {
      // Throw this letter away, it's a duplicate
      delete letter;
      inserted = true;
      break;
    }
  }
  if (!inserted)
    m_inbox.PutDataAtStart(letter);

  //
  // Recalculate our last Known Sequence Id

  for (i = 0; i < m_inbox.Size(); ++i)
  {
    ServerToClientLetter* thisLetter = m_inbox[i];
    if (thisLetter->GetSequenceId() > m_lastValidSequenceIdFromServer + 1)
      break;
    m_lastValidSequenceIdFromServer = thisLetter->GetSequenceId();
  }

  m_inboxMutex.unlock();
}

void NetworkClient::SendLetter(NetworkUpdate* letter)
{
  letter->SetLastSequenceId(m_lastValidSequenceIdFromServer);

  m_outboxMutex.lock();
  m_outbox.PutDataAtEnd(letter);
  m_outboxMutex.unlock();
}

void NetworkClient::ClientJoin()
{
  DebugTrace("CLIENT : Attempting connection...\n");
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::ClientJoin);
  SendLetter(letter);
}

void NetworkClient::ClientLeave()
{
  DebugTrace("CLIENT : Sending disconnect...\n");
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::ClientLeave);
  SendLetter(letter);

  m_lastValidSequenceIdFromServer = -1;
  g_lastProcessedSequenceId = -1;
}

void NetworkClient::RequestTeam(int _teamType, int _desiredId)
{
  DebugTrace("CLIENT : Requesting Team...\n");

  auto letter = NEW NetworkUpdate();
  letter->SetDesiredTeamId(_desiredId);
  letter->SetType(NetworkUpdate::RequestTeam);
  letter->SetTeamType(_teamType);
  SendLetter(letter);
}

void NetworkClient::SendIAmAlive(unsigned char _teamId, const TeamControls& _teamControls)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::Alive);
  letter->SetTeamId(_teamId);
  letter->SetWorldPos(_teamControls.m_mousePos);
  letter->SetTeamControls(_teamControls);
  SendLetter(letter);
}

void NetworkClient::SendSyncronisation(int _lastProcessedId, unsigned char _sync)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::Syncronise);
  letter->SetLastProcessedId(_lastProcessedId);
  letter->SetSync(_sync);
  SendLetter(letter);
}

void NetworkClient::RequestPause()
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::Pause);
  SendLetter(letter);
}

void NetworkClient::RequestSelectUnit(unsigned char _teamId, int _unitId, int _entityId, int _buildingId)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::SelectUnit);
  letter->SetTeamId(_teamId);
  letter->SetUnitId(_unitId);
  letter->SetEntityId(_entityId);
  letter->SetBuildingID(_buildingId);
  SendLetter(letter);
}

void NetworkClient::RequestCreateUnit(unsigned char _teamId, EntityType _troopType, int _numToCreate, int _buildingId)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::CreateUnit);
  letter->SetTeamId(_teamId);
  letter->SetEntityType(_troopType);
  letter->SetNumTroops(_numToCreate);
  letter->SetBuildingID(_buildingId);
  SendLetter(letter);
}

void NetworkClient::RequestCreateUnit(unsigned char _teamId, EntityType _troopType, int _numToCreate, const LegacyVector3& _pos)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::CreateUnit);
  letter->SetTeamId(_teamId);
  letter->SetEntityType(_troopType);
  letter->SetNumTroops(_numToCreate);
  letter->SetBuildingID(-1);
  letter->SetWorldPos(_pos);
  SendLetter(letter);
}

void NetworkClient::RequestAimBuilding(unsigned char _teamId, int _buildingId, const LegacyVector3& _pos)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::AimBuilding);
  letter->SetTeamId(_teamId);
  letter->SetBuildingID(_buildingId);
  letter->SetWorldPos(_pos);
  SendLetter(letter);
}

void NetworkClient::RequestToggleFence(int _buildingId)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::ToggleLaserFence);
  letter->SetBuildingID(_buildingId);
  SendLetter(letter);
}

void NetworkClient::RequestRunProgram(unsigned char _teamId, unsigned char _program)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::RunProgram);
  letter->SetTeamId(_teamId);
  letter->SetProgram(_program);
  SendLetter(letter);
}

void NetworkClient::RequestTargetProgram(unsigned char _teamId, unsigned char _program, const LegacyVector3& _pos)
{
  auto letter = NEW NetworkUpdate();
  letter->SetType(NetworkUpdate::TargetProgram);
  letter->SetTeamId(_teamId);
  letter->SetProgram(_program);
  letter->SetWorldPos(_pos);
  SendLetter(letter);
}

void NetworkClient::ProcessServerUpdates(ServerToClientLetter* letter)
{
  DEBUG_ASSERT(letter->m_type == ServerToClientLetter::Update);

  for (auto& update : letter->m_updates)
  {
    switch (update.m_type)
    {
      case NetworkUpdate::Alive:
        g_app->m_location->UpdateTeam(update.m_teamId, update.m_teamControls);
        break;

      case NetworkUpdate::Pause:
        g_app->m_paused = !g_app->m_paused;
        break;

      case NetworkUpdate::SelectUnit:
        g_app->m_location->m_teams[update.m_teamId].SelectUnit(update.m_unitId, update.m_entityId, update.m_buildingId);
        g_app->m_taskManager->SelectTask(WorldObjectId(update.m_teamId, update.m_unitId, update.m_entityId, -1));
        break;

      case NetworkUpdate::CreateUnit:
      {
        Building* building = g_app->m_location->GetBuilding(update.m_buildingId);
        if (building && building->m_buildingType == BuildingType::TypeFactory)
        {
          auto factory = static_cast<Factory*>(building);
          factory->RequestUnit(update.m_entityType, update.m_numTroops);
        }
        else
        {
          DEBUG_ASSERT(update.GetWorldPos() != g_zeroVector);
          int unitId;
          Unit* unit = g_app->m_location->m_teams[update.m_teamId].NewUnit(update.m_entityType, update.m_numTroops, &unitId,
                                                                           update.GetWorldPos());
          g_app->m_location->SpawnEntities(update.GetWorldPos(), update.m_teamId, unitId, update.m_entityType, update.m_numTroops,
                                           g_zeroVector, update.m_numTroops * 2);
        }
        break;
      }

      case NetworkUpdate::AimBuilding:
      {
        Building* building = g_app->m_location->GetBuilding(update.m_buildingId);
        if (building && building->m_id.GetTeamId() == update.m_teamId && building->m_buildingType == BuildingType::TypeRadarDish)
        {
          auto radarDish = static_cast<RadarDish*>(building);
          radarDish->Aim(update.GetWorldPos());
        }
        break;
      }

      case NetworkUpdate::ToggleLaserFence:
      {
        Building* building = g_app->m_location->GetBuilding(update.m_buildingId);
        if (building && building->m_buildingType == BuildingType::TypeLaserFence)
        {
          auto laserfence = static_cast<LaserFence*>(building);
          laserfence->Toggle();
        }
        break;
      }

      case NetworkUpdate::RunProgram:
      {
        g_app->m_taskManager->RunTask(update.m_program);
        break;
      }

      case NetworkUpdate::TargetProgram:
      {
        int programId = update.m_program;
        g_app->m_taskManager->TargetTask(programId, update.GetWorldPos());
      }
    }
  }
}
