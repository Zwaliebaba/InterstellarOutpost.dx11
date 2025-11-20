#pragma once

#define MAX_HOSTNAME_LEN   	256

constexpr int MAX_PACKET_SIZE = { 10000 };          // Maximum size of a UDP packet, but not ideal for a single message

constexpr int UDP_MESSAGE_HEADER_SIZE = { 8 };       // Size of the UDP message header (4 bytes for length, 4 bytes for checksum)
constexpr int MTU_MESSAGE_SIZE = { 1500 };           // Maximum Transmission Unit size for UDP packets
constexpr int UDP_MESSAGE_SIZE = { MTU_MESSAGE_SIZE - UDP_MESSAGE_HEADER_SIZE };

using net_ip_address_t = sockaddr_in;

enum NetRetCode
{
  NetFailed = -1,
  NetOk,
  NetTimedout,
  NetBadArgs,
  NetMoreData,
  NetClientDisconnect,
  NetNotSupported
};

class Packet;

using NetCallBack = std::function<void(const Packet& _udpPacket)>;

// Define portable names for Win32 functions
#define NetGetLastError() 			WSAGetLastError()
#define NetSleep(a) 				Sleep(a)

// Define portable ways to test various conditions
#define NetIsAddrInUse 				(WSAGetLastError() == WSAEADDRINUSE)
#define NetIsSocketError(a) 		(a == SOCKET_ERROR)
#define NetIsBlockingError(a) 		((a == WSAEALREADY) || (a == WSAEWOULDBLOCK) || (a == WSAEINVAL))
#define NetIsConnected(a) 			(a == WSAEISCONN)
#define NetIsReset(a) 				((a == WSAECONNRESET) || (a == WSAESHUTDOWN))

void IpToString(in_addr in, char* newip);

class NetLib
{
  public:
    static bool Startup(); 
    static void Shutdown();
};
