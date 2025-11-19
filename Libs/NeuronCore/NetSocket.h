#pragma once

#include "DataWriter.h"
#include "NetLib.h"

class NetSocket
{
  public:
    NetSocket();
    ~NetSocket();

    NetRetCode StartListening(const NetCallBack& _fnptr, uint16_t _port);

    // Stops the listener after next (non-blocking) accept call
    void StopListening();

    // Close the current socket connection
    void Close();

    NetRetCode SendUDPPacket(net_ip_address_t _dest, const DataWriter& _buffer);

  protected:
    SOCKET m_sockfd;
    bool m_listening;
};
