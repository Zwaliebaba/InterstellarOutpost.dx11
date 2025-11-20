#include "pch.h"
#include "resource.h"
#include "upgrade_port.h"
#include "GameApp.h"
#include "global_world.h"

UpgradePort::UpgradePort()
  : Building(BuildingType::TypeUpgradePort)
{
  Building::SetShape(Resource::GetShape("upgrade_port.shp"));
}

PrimaryUpgradePort::PrimaryUpgradePort()
  : Building(BuildingType::TypePrimaryUpgradePort),
    m_controlTowersOwned(0)
{
  Building::SetShape(Resource::GetShape("primaryupgradeport.shp"));
}

void PrimaryUpgradePort::ReprogramComplete()
{
  m_controlTowersOwned++;

  if (m_controlTowersOwned == 3)
  {
    GlobalBuilding* gb = g_app->m_globalWorld->GetBuilding(m_id.GetUniqueId(), g_app->m_locationId);
    if (gb)
    {
      gb->m_online = true;
      g_app->m_globalWorld->EvaluateEvents();
    }
  }
}
