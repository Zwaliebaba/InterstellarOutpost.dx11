#include "pch.h"
#include "ClientPeer.h"
#include "Packet.h"

ClientPeer::ClientPeer(const char* _ip)
{
  strcpy(m_clientIP, _ip);

  m_clientHost = {};
  m_clientHost.sin_family = AF_INET;
  m_clientHost.sin_port = htons(4001);

  HOSTENT* pHostent = gethostbyname(_ip);
  if (!pHostent)
  {
    DebugTrace("Host address resolution failed for {}", _ip);
  }
  m_clientHost.sin_addr.s_addr = *(unsigned long*)pHostent->h_addr_list[0];

  m_lastKnownSequenceId = -1;
}

void ClientPeer::SendSYN(bool _startup)
{
  RUDPSync syncPacket;
  syncPacket.m_header.m_sequence = m_seqNumber++;
  syncPacket.m_optionFlag = 1; // Set the CHK flag

  if (!_startup)
  {
    
  }

}
