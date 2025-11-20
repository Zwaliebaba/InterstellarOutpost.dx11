#include "pch.h"
#include "global_internet.h"
#include "GameApp.h"
#include "camera.h"
#include "global_world.h"
#include "hi_res_time.h"
#include "main.h"
#include "math_utils.h"
#include "profiler.h"
#include "resource.h"

//#define DISPLAY_LIST_NAME_LINKS "GlobalInternetLinks"
//#define DISPLAY_LIST_NAME_NODES "GlobalInternetNodes"

GlobalInternetNode::GlobalInternetNode()
  : m_size(0),
    m_burst(0),
    m_numLinks(0) {}

void GlobalInternetNode::AddLink(int _id)
{
  DEBUG_ASSERT(m_numLinks < GLOBALINTERNET_MAXNODELINKS);
  m_links[m_numLinks] = _id;
  m_numLinks++;
}

GlobalInternet::GlobalInternet()
  : m_nodes(nullptr),
    m_numNodes(0),
    m_links(nullptr),
    m_numLinks(0),
    m_nearestNodeToCenter(-1),
    m_nearestDistance(FLT_MAX)
{
  darwiniaSeedRandom(1);
  GenerateInternet();
}

GlobalInternet::~GlobalInternet() { DeleteInternet(); }

unsigned short GlobalInternet::GenerateInternet(const LegacyVector3& _pos, unsigned char _size)
{
  GetHighResTime();

  GlobalInternetNode* node = &m_nodes[m_numNodes];
  node->m_pos = _pos;
  node->m_size = _size;
  unsigned short nodeIndex = m_numNodes;
  m_numNodes++;
  DEBUG_ASSERT(m_numNodes < GLOBALINTERNET_MAXNODES);

  float distanceToCenter = _pos.Mag();
  if (distanceToCenter < m_nearestDistance)
  {
    m_nearestDistance = distanceToCenter;
    m_nearestNodeToCenter = nodeIndex;
  }

  unsigned char numLinks = _size;
  float distance = powf(_size, 4.0f) * 2.0f;

  while (numLinks > 0)
  {
    float z = sfrand(distance);
    float y = sfrand(distance);
    float x = sfrand(distance);
    LegacyVector3 newPos = _pos + LegacyVector3(x, y, z);
    unsigned short newIndex = GenerateInternet(newPos, _size - 1);
    m_links[m_numLinks].m_from = nodeIndex;
    m_links[m_numLinks].m_to = newIndex;
    m_links[m_numLinks].m_size = _size;

    m_nodes[newIndex].AddLink(m_numLinks);
    node->AddLink(m_numLinks);

    m_numLinks++;
    DEBUG_ASSERT(m_numLinks <= GLOBALINTERNET_MAXLINKS);

    --numLinks;
  }

  return nodeIndex;
}

void GlobalInternet::GenerateInternet()
{
  m_links = NEW GlobalInternetLink[GLOBALINTERNET_MAXLINKS];
  m_nodes = NEW GlobalInternetNode[GLOBALINTERNET_MAXNODES];

  LegacyVector3 center(-797, 1949, -1135);
  unsigned short firstNode = GenerateInternet(center, GLOBALINTERNET_ITERATIONS);

  m_nodes[m_numNodes].m_pos.Zero();
  m_nodes[m_numNodes].m_size = 0.0f;
  unsigned short nodeIndex = m_numNodes;
  m_numNodes++;
  DEBUG_ASSERT(m_numNodes <= GLOBALINTERNET_MAXNODES);

  m_links[m_numLinks].m_from = m_nearestNodeToCenter;
  m_links[m_numLinks].m_to = nodeIndex;
  m_links[m_numLinks].m_size = 1.0f;
  m_numLinks++;
  DEBUG_ASSERT(m_numLinks <= GLOBALINTERNET_MAXLINKS);

  for (int i = 0; i < m_numNodes; ++i)
  {
    GlobalInternetNode* node = &m_nodes[i];
    if (node->m_numLinks == 1)
      m_leafs.PutData(i);
  }
}

void GlobalInternet::DeleteInternet()
{
  delete [] m_nodes;
  m_numNodes = 0;
  delete [] m_links;
  m_numLinks = 0;
  m_leafs.Empty();
  m_bursts.Empty();

  //Resource::DeleteDisplayList(DISPLAY_LIST_NAME_LINKS);
  //Resource::DeleteDisplayList(DISPLAY_LIST_NAME_NODES);
}

void GlobalInternet::Render()
{
  START_PROFILE(g_app->m_profiler, "Internet");

  /*static*/
  float scale = 1000.0f;

  glPushMatrix();
  glScalef(scale, scale, scale);

  float fog = 0.0f;
  float fogCol[] = {fog, fog, fog, fog};

  int fogVal = 5810000;

  glFogf(GL_FOG_DENSITY, 1.0f);
  glFogf(GL_FOG_START, 0.0f);
  glFogf(GL_FOG_END, static_cast<float>(fogVal));
  glFogfv(GL_FOG_COLOR, fogCol);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glEnable(GL_FOG);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDepthMask(false);
  glDisable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\laserfence2.bmp"));
    glColor4f(0.25f, 0.25f, 0.5f, 0.8f);

    //
    // Render Links

    glBegin(GL_QUADS);

    for (int i = 0; i < m_numLinks; ++i)
    {
      GlobalInternetLink* link = &m_links[i];
      GlobalInternetNode* from = &m_nodes[link->m_from];
      GlobalInternetNode* to = &m_nodes[link->m_to];

      LegacyVector3 fromPos = from->m_pos;
      LegacyVector3 toPos = to->m_pos;
      LegacyVector3 midPoint = (toPos + fromPos) / 2.0f;
      LegacyVector3 camToMidPoint = g_zeroVector - midPoint;
      LegacyVector3 rightAngle = (camToMidPoint ^ (midPoint - toPos)).Normalize();

      rightAngle *= link->m_size * 0.5f;

      glTexCoord2i(0, 0);
      glVertex3fv((fromPos - rightAngle).GetData());
      glTexCoord2i(0, 1);
      glVertex3fv((fromPos + rightAngle).GetData());
      glTexCoord2i(1, 1);
      glVertex3fv((toPos + rightAngle).GetData());
      glTexCoord2i(1, 0);
      glVertex3fv((toPos - rightAngle).GetData());
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\glow.bmp"));

    glColor4f(0.8f, 0.8f, 1.0f, 0.6f);
    float nodeSize = 10.0f;

    glBegin(GL_QUADS);
    for (int i = 0; i < m_numNodes; ++i)
    {
      GlobalInternetNode* node = &m_nodes[i];

      LegacyVector3 camToMidPoint = (g_zeroVector - node->m_pos).Normalize();
      LegacyVector3 right = camToMidPoint ^ LegacyVector3(0, 0, 1);
      LegacyVector3 up = right ^ camToMidPoint;
      right *= nodeSize;
      up *= nodeSize;
      glTexCoord2i(0, 0);
      glVertex3fv((node->m_pos - up - right).GetData());
      glTexCoord2i(1, 0);
      glVertex3fv((node->m_pos - up + right).GetData());
      glTexCoord2i(1, 1);
      glVertex3fv((node->m_pos + up + right).GetData());
      glTexCoord2i(0, 1);
      glVertex3fv((node->m_pos + up - right).GetData());
    }
    glEnd();

  //  glEndList();
  //}

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);
  glDepthMask(true);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);

  RenderPackets();

  g_app->m_globalWorld->SetupFog();
  glDisable(GL_FOG);

  glPopMatrix();

  END_PROFILE(g_app->m_profiler, "Internet");
}

void GlobalInternet::TriggerPacket(unsigned short _nodeId, unsigned short _fromLinkId)
{
  GlobalInternetNode* newNode = &m_nodes[_nodeId];

  if (newNode->m_numLinks == 1 && newNode->m_links[0] == _fromLinkId)
    return;

  while (true)
  {
    int newLinkIndex = darwiniaRandom() % newNode->m_numLinks;
    if (newNode->m_links[newLinkIndex] != _fromLinkId)
    {
      GlobalInternetLink* newLink = &m_links[newNode->m_links[newLinkIndex]];
      if (newLink->m_from == _nodeId)
        newLink->m_packets.PutData(1.0f);
      else
        newLink->m_packets.PutData(-1.0f);
      break;
    }
  }
}

void GlobalInternet::RenderPackets()
{
  //
  // Create new packets

  if (frand(100.0f) < 11.0f)
  {
    int leafIndex = frand(m_leafs.Size());
    GlobalInternetNode* node = &m_nodes[m_leafs[leafIndex]];
    node->m_burst = 4.0f;
    m_bursts.PutData(m_leafs[leafIndex]);
  }

  //
  // Deal with data bursts

  for (int i = 0; i < m_bursts.Size(); ++i)
  {
    GlobalInternetNode* node = &m_nodes[m_bursts[i]];
    node->m_burst -= g_advanceTime;
    if (node->m_burst > 0.0f)
      TriggerPacket(m_bursts[i], -1);
    else
    {
      m_bursts.RemoveData(i);
      --i;
    }
  }

  //
  // Advance / render all packets

  float packetSize = 30.0f;
  LegacyVector3 camRight = g_app->m_camera->GetRight() * packetSize;
  LegacyVector3 camUp = g_app->m_camera->GetUp() * packetSize;
  float posChange = g_advanceTime;

  glColor4f(0.25f, 0.25f, 0.5f, 0.8f);

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures\\starburst.bmp"));
  glDepthMask(false);

  for (int i = 0; i < m_numLinks; ++i)
  {
    GlobalInternetLink* link = &m_links[i];
    for (int j = 0; j < link->m_packets.Size(); ++j)
    {
      float* thisPacket = link->m_packets.GetPointer(j);
      float packetVal = *thisPacket;
      if (*thisPacket > 0.0f)
      {
        *thisPacket -= posChange;
        if (*thisPacket <= 0.01f)
        {
          *thisPacket = 0.01f;
          TriggerPacket(link->m_to, i);
          link->m_packets.RemoveData(j);
          --j;
        }
      }
      else if (*thisPacket < 0.0f)
      {
        *thisPacket += posChange;
        if (*thisPacket >= -0.01f)
        {
          *thisPacket = -0.01f;
          TriggerPacket(link->m_from, i);
          link->m_packets.RemoveData(j);
          --j;
        }
      }

      GlobalInternetNode* from = &m_nodes[link->m_from];
      GlobalInternetNode* to = &m_nodes[link->m_to];
      LegacyVector3 packetPos;
      if (packetVal > 0.0f)
        packetPos = to->m_pos + (from->m_pos - to->m_pos) * packetVal;
      else if (packetVal < 0.0f)
        packetPos = from->m_pos + (to->m_pos - from->m_pos) * -packetVal;

      glBegin(GL_QUADS);
      glTexCoord2i(0, 0);
      glVertex3fv((packetPos - camUp - camRight).GetData());
      glTexCoord2i(1, 0);
      glVertex3fv((packetPos - camUp + camRight).GetData());
      glTexCoord2i(1, 1);
      glVertex3fv((packetPos + camUp + camRight).GetData());
      glTexCoord2i(0, 1);
      glVertex3fv((packetPos + camUp - camRight).GetData());
      glEnd();
    }
  }

  glDepthMask(true);
  glDisable(GL_TEXTURE_2D);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
