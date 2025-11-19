#include "pch.h"
#include "powerstation.h"
#include "GameApp.h"
#include "building.h"
#include "laserfence.h"
#include "location.h"
#include "resource.h"
#include "text_stream_readers.h"

Powerstation::Powerstation()
  : Building(BuildingType::TypePowerstation),
    m_linkedBuildingId(-1)
{
  Building::SetShape(Resource::GetShape("powerstation.shp"));
}

// *** Initialize
void Powerstation::Initialize(Building* _template)
{
  Building::Initialize(_template);
  DEBUG_ASSERT(_template->m_buildingType == BuildingType::TypePowerstation);
  m_linkedBuildingId = static_cast<Powerstation*>(_template)->m_linkedBuildingId;
}

// *** Advance
bool Powerstation::Advance()
{
  Building* b = g_app->m_location->GetBuilding(m_linkedBuildingId);
  if (b->m_buildingType == BuildingType::TypeLaserFence)
  {
    auto fence = static_cast<LaserFence*>(b);
    if (!fence->IsEnabled() && GetNumPorts() == GetNumPortsOccupied())
      fence->Enable();

    if (fence->IsEnabled() && GetNumPortsOccupied() <= GetNumPorts() * 3.0f / 4.0f)
      fence->Disable();
  }

  return Building::Advance();
}

void Powerstation::Render(float predictionTime) { Building::Render(predictionTime); }

int Powerstation::GetBuildingLink() { return m_linkedBuildingId; }

void Powerstation::SetBuildingLink(int _buildingId) { m_linkedBuildingId = _buildingId; }

void Powerstation::Read(TextReader* _in, bool _dynamic)
{
  Building::Read(_in, _dynamic);

  char* word = _in->GetNextToken();
  m_linkedBuildingId = atoi(word);
}

