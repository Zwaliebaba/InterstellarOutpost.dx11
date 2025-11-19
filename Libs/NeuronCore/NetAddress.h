#pragma once

#undef SetPort

struct IPv6AddressBytes
{
	IPv6AddressBytes() = default;
	uint8_t m_bytes[16] = {};
};

class NetAddress
{
public:
	NetAddress();
	NetAddress(uint32_t address, uint16_t port);
	NetAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);
	NetAddress(const IPv6AddressBytes* ipv6_bytes, uint16_t port);

	bool operator==(const NetAddress& address) const;
	bool operator!=(const NetAddress& address) const { return !(*this == address); }

	int getFamily() const { return m_addrFamily; }
	bool isValid() const { return m_addrFamily != 0; }
	bool isIPv6() const { return m_addrFamily == AF_INET6; }
	in_addr getAddress() const { return m_address.m_ipv4; }
	in6_addr getAddress6() const { return m_address.m_ipv6; }
	uint16_t getPort() const { return m_port; }

	void print(std::ostream& s) const;
	std::string serializeString() const;

	// Is this an address that binds to all interfaces (like INADDR_ANY)?
	bool isAny() const;
	// Is this an address referring to the local host?
	bool IsLocalhost() const;

	// `name`: hostname or numeric IP
	// `fallback`: fallback IP to try gets written here
	// any empty name resets the IP to the "any address"
	// may throw ResolveError (address is unchanged in this case)
	void Resolve(const char* _name, NetAddress* fallback = nullptr);

	void SetAddress(uint32_t address);
	void setAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
	void SetAddress(const IPv6AddressBytes* ipv6_bytes);
	void SetPort(uint16_t port);

private:
	unsigned short m_addrFamily = 0;
	union
	{
		in_addr m_ipv4;
		in6_addr m_ipv6;
	} m_address;
	// port is separate from in_addr structures
	uint16_t m_port = 0;
};

