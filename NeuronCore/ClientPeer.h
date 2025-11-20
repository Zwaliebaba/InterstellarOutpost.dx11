#pragma once

#include "NetSocket.h"

enum class ConnectState : uint8_t
{
  INIT,
  LISTEN,
  ESTABLISHED,
  HALF_ESTABLISHED,
  FINISHED
};

class ClientPeer
{
  public:
    ClientPeer(const char* _ip);

    const net_ip_address_t& GetHost() const { return m_clientHost; }
    char* GetIP() { return m_clientIP; }

    int m_lastKnownSequenceId;

  protected:
    void SendSYN(bool _startup = false);
    void SendACK();
    void SendData();

    //Concurrency::concurrent_queue<Packet> m_sendList;
    //Concurrency::concurrent_queue<Packet> m_receiveList;
    //Concurrency::concurrent_unordered_map<uint8_t, Packet> m_outList;

    uint8_t m_seqNumber = 1;                        // the last number to send
    uint8_t m_ackNumber = 0;                        // the next ack number to send i.e. the last number to receive

    ConnectState m_connectState = ConnectState::INIT;

    net_ip_address_t m_clientHost;
    char m_clientIP[32];
};
