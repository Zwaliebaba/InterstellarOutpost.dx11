#include "pch.h"
#include "NetSocket.h"
#include "DataWriter.h"
#include "Packet.h"

NetSocket::NetSocket()
  : m_listening(false)
{
  m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_sockfd == (SOCKET)INVALID_SOCKET)
    Fatal("Failed to create Socket");
}

NetSocket::~NetSocket()
{
  Close();
}

void NetSocket::Close()
{
  if (m_sockfd != INVALID_SOCKET)
  {
    closesocket(m_sockfd);
    m_sockfd = INVALID_SOCKET;
  }
}

NetRetCode NetSocket::StartListening(const NetCallBack& _fnptr, uint16_t _port)
{
  NetRetCode ret = NetOk;

  // Allow fast reuse (not strictly necessary for UDP here).
  BOOL opt = TRUE;
  setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

  net_ip_address_t servaddr = {};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(_port);

  // Signal that we should be listening
  m_listening = true;

  // Bind socket to port
  if (int iResult = bind(m_sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)); iResult != 0)
    Fatal(L"bind failed with error {}", WSAGetLastError());

  uint8_t buf[MAX_PACKET_SIZE];
  net_ip_address_t clientaddr = {};
  int cliLen = static_cast<int>(sizeof(clientaddr));

  while (m_listening)
  {
    const int received = recvfrom(m_sockfd, reinterpret_cast<char*>(buf), MAX_PACKET_SIZE, 0, (sockaddr*)&clientaddr, &cliLen);
    if (received == SOCKET_ERROR)
      return NetFailed;

    // Call function pointer with datagram data (type is Packet) -
    _fnptr(Packet(clientaddr, buf, received));
  }
  return ret;
}

void NetSocket::StopListening()
{
  m_listening = false;
}

// Write method using select
NetRetCode NetSocket::SendUDPPacket(net_ip_address_t _dest, const DataWriter& _buffer)
{
  DEBUG_ASSERT(m_sockfd != INVALID_SOCKET);

  auto buf = _buffer.Data();
  int size = static_cast<int>(_buffer.Size());
  if (size == 0)
    return NetOk;

  int bytesSent = sendto(m_sockfd, reinterpret_cast<const char*>(buf), size, 0, reinterpret_cast<sockaddr*>(&_dest), sizeof(_dest));
  if (bytesSent == SOCKET_ERROR)
  {
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS)
      return NetMoreData;

    DebugTrace(L"SendUDPPacket sendto failed with error {}", err);
    shutdown(m_sockfd, SD_SEND);
    return NetFailed;
  }

  return NetOk;
}
