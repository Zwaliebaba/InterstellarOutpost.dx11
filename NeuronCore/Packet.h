#pragma once

#include "NetLib.h"

enum class PacketType : uint8_t
{
  UNDEFINED,
  DATA,
  SYN,
  SYNACK,
  ACK,
  WND_REQUEST,
  WND_INFO
};

#pragma pack (push, 1)

enum class RUDPControl : uint8_t
{
  NONE = 0,
  SYN = 1,         // Synchronization segment is present
  ACK = 2,         // Acknowledgment number in the header is valid
  EAK = 4,         // Extended acknowledge segment is present
  RST = 8,         // Reset segment is present
  NUL = 16,        // null segment is present
  CHK = 32,        // Checksum segment is present
  TCS = 64,        // Transfer connection state segment is present
};
DEFINE_ENUM_FLAG_OPERATORS(RUDPControl);

struct RUDPHeader
{
  RUDPControl m_control;
  uint8_t m_size;
  uint8_t m_sequence;
  uint8_t m_ack;
};

struct RUDPSync
{
  RUDPSync()
  {
    m_header.m_control = RUDPControl::SYN;
    m_header.m_size = sizeof(RUDPSync);
  }

  RUDPHeader m_header;

  uint8_t m_version = 1;              // Protocol version
  uint8_t m_maxSegmentSize = 0;           // Maximum segment (payload) size (bytes)
  uint8_t m_optionFlag = 0;
  uint8_t m_spare = 0;
  uint16_t m_windowSize = 0;              // Window size (bytes)
  uint16_t m_retransTimeout = 0;
  uint16_t m_cumAckTimeout = 0;
  uint16_t m_nullTimeout = 0;
  uint16_t m_transferTimeout = 0;
  uint8_t m_maxRetransmissions = 0;       // Maximum number of retransmissions
  uint8_t m_maxCumACK = 0;
  uint8_t m_maxOutofSeq = 0;
  uint8_t m_maxAutoReset = 0;
  uint32_t m_connectionID = 0;         
  uint16_t m_checksum = 0;
};
static_assert(sizeof(RUDPSync) == 28, "RUDPSync size mismatch");

struct UDPAck
{
  RUDPHeader m_header;
  uint16_t m_checksum;             // Checksum of the packet
};
static_assert(sizeof(UDPAck) == 6, "UDPAck size mismatch");

struct UDPReset
{
  RUDPHeader m_header;
  uint16_t m_checksum;             // Checksum of the packet
};
static_assert(sizeof(UDPReset) == 6, "UDPAck size mismatch");

struct UDPNull
{
  RUDPHeader m_header;
  uint16_t m_checksum;             // Checksum of the packet
};
static_assert(sizeof(UDPNull) == 6, "UDPAck size mismatch");

#pragma pack (pop)

class Packet
{
public:
  Packet(const net_ip_address_t& _clientaddr, uint8_t* _buffer, size_t _length);

  // Copy / move support (const correctness required for concurrent_queue)
  Packet& operator=(const Packet& rhs) = default;

  const net_ip_address_t* GetClientAddress() const { return &m_clientAddress; }
  void SetClientAddress(const net_ip_address_t& _clientAddress) { m_clientAddress = _clientAddress; }

  uint8_t* GetBuffer() const { return m_buffer.get(); }
  size_t GetSize() const { return m_size; }

protected:
  PacketType m_type;

  net_ip_address_t m_clientAddress;
  std::shared_ptr<uint8_t[]> m_buffer;
  size_t m_size;
};
