#include "pch.h"
#include "NetAddress.h"
#include "NetworkExceptions.h"

NetAddress::NetAddress() { memset(&m_address, 0, sizeof(m_address)); }

NetAddress::NetAddress(uint32_t address, uint16_t port)
{
  memset(&m_address, 0, sizeof(m_address));
  SetAddress(address);
  SetPort(port);
}

NetAddress::NetAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
{
  memset(&m_address, 0, sizeof(m_address));
  setAddress(a, b, c, d);
  SetPort(port);
}

NetAddress::NetAddress(const IPv6AddressBytes* ipv6_bytes, uint16_t port)
{
  memset(&m_address, 0, sizeof(m_address));
  SetAddress(ipv6_bytes);
  SetPort(port);
}

// Equality (address family, IP and port must be equal)
bool NetAddress::operator==(const NetAddress& other) const
{
  if (other.m_addrFamily != m_addrFamily || other.m_port != m_port)
    return false;

  if (m_addrFamily == AF_INET)
    return m_address.m_ipv4.s_addr == other.m_address.m_ipv4.s_addr;

  if (m_addrFamily == AF_INET6)
    return memcmp(m_address.m_ipv6.s6_addr, other.m_address.m_ipv6.s6_addr, 16) == 0;

  return false;
}

void NetAddress::Resolve(const char* _name, NetAddress* fallback)
{
  if (!_name || _name[0] == 0)
  {
    if (m_addrFamily == AF_INET)
      SetAddress(static_cast<uint32_t>(0));
    else if (m_addrFamily == AF_INET6)
      SetAddress(nullptr);
    if (fallback)
      *fallback = NetAddress();
    return;
  }

  const auto& copy_from_ai = [](const struct addrinfo* ai, NetAddress* to)
  {
    if (ai->ai_family == AF_INET)
    {
      auto t = reinterpret_cast<struct sockaddr_in*>(ai->ai_addr);
      to->m_addrFamily = AF_INET;
      to->m_address.m_ipv4 = t->sin_addr;
    }
    else if (ai->ai_family == AF_INET6)
    {
      auto t = reinterpret_cast<struct sockaddr_in6*>(ai->ai_addr);
      to->m_addrFamily = AF_INET6;
      to->m_address.m_ipv6 = t->sin6_addr;
    }
    else
      to->m_addrFamily = 0;
  };

  addrinfo hints = {};

  // set a type, so every unique address is only returned once
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_ADDRCONFIG;

  // Do getaddrinfo()
  addrinfo* resolved = nullptr;
  int e = getaddrinfo(_name, nullptr, &hints, &resolved);
  if (e != 0)
    throw ResolveError(gai_strerrorA(e));
  assert(resolved);

  // Copy data
  copy_from_ai(resolved, this);
  if (fallback)
  {
    *fallback = NetAddress();
    if (resolved->ai_next)
      copy_from_ai(resolved->ai_next, fallback);
  }

  freeaddrinfo(resolved);
}

// IP address -> textual representation
std::string NetAddress::serializeString() const
{
  char str[INET6_ADDRSTRLEN];
  if (inet_ntop(m_addrFamily, &m_address, str, sizeof(str)) == nullptr)
    return "";
  return str;
}

bool NetAddress::isAny() const
{
  if (m_addrFamily == AF_INET)
    return m_address.m_ipv4.s_addr == 0;
  if (m_addrFamily == AF_INET6)
  {
    static constexpr char zero[16] = {0};
    return memcmp(m_address.m_ipv6.s6_addr, zero, 16) == 0;
  }

  return false;
}

void NetAddress::SetAddress(uint32_t address)
{
  m_addrFamily = AF_INET;
  m_address.m_ipv4.s_addr = htonl(address);
}

void NetAddress::setAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  uint32_t addr = (a << 24) | (b << 16) | (c << 8) | d;
  SetAddress(addr);
}

void NetAddress::SetAddress(const IPv6AddressBytes* ipv6_bytes)
{
  m_addrFamily = AF_INET6;
  if (ipv6_bytes)
    memcpy(m_address.m_ipv6.s6_addr, ipv6_bytes->m_bytes, 16);
  else
    memset(m_address.m_ipv6.s6_addr, 0, 16);
}

void NetAddress::SetPort(uint16_t port) { m_port = port; }

void NetAddress::print(std::ostream& s) const
{
  if (m_addrFamily == AF_INET6)
    s << "[" << serializeString() << "]:" << m_port;
  else if (m_addrFamily == AF_INET)
    s << serializeString() << ":" << m_port;
  else
    s << "(undefined)";
}

bool NetAddress::IsLocalhost() const
{
  if (m_addrFamily == AF_INET6)
  {
    static const uint8_t localhost_bytes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    static const uint8_t mapped_ipv4_localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 0};

    auto* addr = m_address.m_ipv6.s6_addr;

    return memcmp(addr, localhost_bytes, 16) == 0 || memcmp(addr, mapped_ipv4_localhost, 13) == 0;
  }
  if (m_addrFamily == AF_INET)
  {
    auto addr = ntohl(m_address.m_ipv4.s_addr);
    return (addr >> 24) == 0x7f;
  }
  return false;
}
