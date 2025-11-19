#include "pch.h"
#include "NetLib.h"

void IpToString(in_addr in, char* newip)
{
  sprintf(newip, "%u.%u.%u.%u", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
}

bool NetLib::Startup()
{
  WSADATA wsaData;

  WORD versionRequested = MAKEWORD(2, 2);

  if (WSAStartup(versionRequested, &wsaData))
  {
    DebugTrace("WinSock startup failed");
    return false;
  }

  // Confirm that the WinSock DLL supports 2.2. Note that if the DLL supports
  // versions greater than 2.2 in addition to 2.2, it will still return
  // 2.2 in wVersion since that is the version we requested.                                       
  if ((LOBYTE(wsaData.wVersion) != 2) || (HIBYTE(wsaData.wVersion) != 2))
  {
    // Tell the user that we could not find a usable WinSock DLL
    WSACleanup();
    DebugTrace("No valid WinSock DLL found");
    return false;
  }

  return true;
}

void NetLib::Shutdown()
{
  WSACleanup();
}
