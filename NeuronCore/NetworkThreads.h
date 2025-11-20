#pragma once

#include "Connection.h"

class ConnectionSendThread : public Tasks::Thread
{
  public:
    ConnectionSendThread(unsigned int max_packet_size, float timeout);

    void* run() override;

    void Trigger();

    void setParent(Connection* parent)
    {
      DEBUG_ASSERT(parent != NULL); // Pre-condition
      m_connection = parent;
    }

    void setPeerTimeout(float peer_timeout) { m_timeout = peer_timeout; }

  private:
    void runTimeouts(float dtime, uint32_t peer_packet_quota);
    void resendReliable(Channel& channel, const BufferedPacket* k, float resend_timeout);
    void rawSend(const BufferedPacket* p);
    bool rawSendAsPacket(session_t peer_id, uint8_t channelnum, const SharedBuffer<uint8_t>& data, bool reliable);

    void processReliableCommand(ConnectionCommandPtr& c);
    void processNonReliableCommand(ConnectionCommandPtr& c);
    void serve(NetAddress bind_address);
    void connect(NetAddress address);
    void disconnect();
    void disconnect_peer(session_t peer_id);
    void fix_peer_id(session_t own_peer_id);
    void send(session_t peer_id, uint8_t channelnum, const SharedBuffer<uint8_t>& data);
    void sendReliable(ConnectionCommandPtr& c);
    void sendToAll(uint8_t channelnum, const SharedBuffer<uint8_t>& data);
    void sendToAllReliable(ConnectionCommandPtr& c);

    void sendPackets(float dtime, uint32_t peer_packet_quota);

    void sendAsPacket(session_t peer_id, uint8_t channelnum, const SharedBuffer<uint8_t>& data, bool ack = false);

    void sendAsPacketReliable(BufferedPacketPtr& p, Channel* channel);

    bool packetsQueued();

    Connection* m_connection = nullptr;
    unsigned int m_max_packet_size;
    float m_timeout;
    std::queue<OutgoingPacket> m_outgoing_queue;
    HANDLE m_send_sleep_semaphore;

    unsigned int m_iteration_packets_avaialble;
    unsigned int m_max_data_packets_per_iteration;
    unsigned int m_max_packets_requeued = 256;
};

class ConnectionReceiveThread : public Tasks::Thread
{};
