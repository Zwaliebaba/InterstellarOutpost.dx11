#include "pch.h"
#include "Packet.h"

Packet::Packet(const net_ip_address_t& _clientaddr, uint8_t* _buffer, const size_t _length)
  : m_type(PacketType::UNDEFINED),
    m_size(_length)
{
  m_clientAddress = _clientaddr;
  m_buffer = std::make_shared<uint8_t[]>(_length);
  std::copy_n(_buffer, _length, m_buffer.get());
}
