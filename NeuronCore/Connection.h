#pragma once

#include "NetAddress.h"

typedef uint16_t session_t;

enum ConnectionEventType {
	CONNEVENT_NONE,
	CONNEVENT_DATA_RECEIVED,
	CONNEVENT_PEER_ADDED,
	CONNEVENT_PEER_REMOVED,
	CONNEVENT_BIND_FAILED,
};

struct ConnectionEvent : NonCopyable
{
	ConnectionEventType m_type;
	session_t m_peerId = 0;
	std::vector<uint8_t> m_data;
	bool m_timeout = false;
	NetAddress m_address;
};

class Connection
{
};

