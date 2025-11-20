#ifndef INCLUDED_GLOBAL_INTERNET
#define INCLUDED_GLOBAL_INTERNET

#include "LegacyVector3.h"
#include "fast_darray.h"
#include "llist.h"

#define GLOBALINTERNET_ITERATIONS       7
#define GLOBALINTERNET_MAXNODES		    13701
#define GLOBALINTERNET_MAXLINKS		    13700
#define GLOBALINTERNET_MAXNODELINKS     GLOBALINTERNET_ITERATIONS

// ****************************************************************************
// Class GlobalInternetNode
// ****************************************************************************

class GlobalInternetNode
{
  public:
    GlobalInternetNode();
    void AddLink(int id);

    LegacyVector3 m_pos;
    unsigned char m_size;
    float m_burst;

    unsigned short m_links[GLOBALINTERNET_ITERATIONS];
    unsigned short m_numLinks;
};

//*****************************************************************************
// Class GlobalInternetLink
//*****************************************************************************

class GlobalInternetLink
{
  public:
    unsigned short m_from;
    unsigned short m_to;
    float m_size;
    LList<float> m_packets;
};

//*****************************************************************************
// Class GlobalInternet
//*****************************************************************************

class GlobalInternet
{
  protected:
    GlobalInternetNode* m_nodes;
    unsigned short m_numNodes;
    GlobalInternetLink* m_links;
    unsigned short m_numLinks;
    LList<int> m_leafs;
    LList<int> m_bursts;

    int m_nearestNodeToCenter;
    float m_nearestDistance;

    void GenerateInternet();
    unsigned short GenerateInternet(const LegacyVector3& _pos, unsigned char _size);
    void DeleteInternet();

    void TriggerPacket(unsigned short _nodeId, unsigned short _fromLinkId);

  public:
    GlobalInternet();
    ~GlobalInternet();

    void Render();
    void RenderPackets();
};

#endif
