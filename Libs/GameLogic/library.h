#ifndef _included_library_h
#define _included_library_h

#include "building.h"

#include "global_world.h"

class Library : public Building
{
  public:
    bool m_scrollSpawned[GlobalResearch::NumResearchItems];

    Library();

    bool Advance() override;
};

#endif
