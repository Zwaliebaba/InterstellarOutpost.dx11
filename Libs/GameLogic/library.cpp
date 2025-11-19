#include "pch.h"
#include "resource.h"
#include "shape.h"
#include "library.h"
#include "researchitem.h"
#include "GameApp.h"
#include "location.h"

Library::Library()
  : Building(BuildingType::TypeLibrary)
{
  Building::SetShape(Resource::GetShape("library.shp"));
  memset(m_scrollSpawned, 0, GlobalResearch::NumResearchItems * sizeof(bool));
}

bool Library::Advance()
{
  for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
  {
    if (!m_scrollSpawned[i] && g_app->m_globalWorld->m_research->HasResearch(i))
    {
      char markerName[256];
      sprintf(markerName, "MarkerResearch%02d", i + 1);
      ShapeMarker* scrollMarker = m_shape->m_rootFragment->LookupMarker(markerName);
      DEBUG_ASSERT(scrollMarker);

      Matrix34 rootMat(m_front, g_upVector, m_pos);
      Matrix34 scrollPos = scrollMarker->GetWorldMatrix(rootMat);

      auto item = new ResearchItem();
      item->m_researchType = i;
      item->m_inLibrary = true;
      item->m_pos = scrollPos.pos;
      item->m_id.SetUniqueId(g_app->m_globalWorld->GenerateBuildingId());
      g_app->m_location->m_buildings.PutData(item);

      m_scrollSpawned[i] = true;
    }
  }

  return Building::Advance();
}
