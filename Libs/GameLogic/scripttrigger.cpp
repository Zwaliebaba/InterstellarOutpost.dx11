#include "pch.h"
#include "scripttrigger.h"
#include "GameApp.h"
#include "camera.h"
#include "entity_grid.h"
#include "globals.h"
#include "location.h"
#include "Script.h"
#include "text_stream_readers.h"

ScriptTrigger::ScriptTrigger()
  : Building(BuildingType::TypeScriptTrigger),
    m_range(100.0f),
    m_entityType(SCRIPTRIGGER_RUNNEVER),
    m_linkId(-1),
    m_triggered(0),
    m_timer(0.0f)
{
  sprintf(m_scriptFilename, "NewScript");
}

void ScriptTrigger::Initialize(Building* _template)
{
  Building::Initialize(_template);

  auto trigger = static_cast<ScriptTrigger*>(_template);
  strcpy(m_scriptFilename, trigger->m_scriptFilename);
  m_range = trigger->m_range;
  m_entityType = trigger->m_entityType;
  m_linkId = trigger->m_linkId;
}

void ScriptTrigger::Trigger()
{
  if (strstr(m_scriptFilename, ".txt"))
  {
    // Run a script, speficied by filename
    g_app->m_script->RunScript(m_scriptFilename);
    m_triggered = -1;
  }
  else
    m_triggered = 1;
}

bool ScriptTrigger::Advance()
{
  if (m_entityType != SCRIPTRIGGER_RUNNEVER)
  {
    if (m_triggered == -1)
    {
      // Our script has been run, we are done
      return true;
    }
    if (m_triggered > 0) {}
    else
    {
      bool alreadyRunningScript = g_app->m_script->IsRunningScript() || !g_app->m_camera->IsInteractive();

      if (!alreadyRunningScript)
      {
        // We haven't been triggered yet
        m_timer -= SERVER_ADVANCE_PERIOD;

        if (m_timer <= 0.0f)
        {
          m_timer = 1.0f;

          if (m_entityType == SCRIPTRIGGER_RUNALWAYS)
            Trigger();
          else if (m_entityType == SCRIPTRIGGER_RUNCAMENTER)
          {
            float camDistance = (g_app->m_camera->GetPos() - m_pos).Mag();
            LegacyVector3 camVel = g_app->m_camera->GetVel();
            bool camInteractive = g_app->m_camera->IsInteractive();

            if (camDistance <= m_range && camVel.Mag() < 5.0f && camInteractive)
              Trigger();
          }
          else if (m_entityType == SCRIPTRIGGER_RUNCAMVIEW)
          {
            float camDistance = (g_app->m_camera->GetPos() - m_pos).Mag();
            LegacyVector3 camVel = g_app->m_camera->GetVel();
            bool camInteractive = g_app->m_camera->IsInteractive();
            bool inView = RaySphereIntersection(g_app->m_camera->GetPos(), g_app->m_camera->GetFront(), m_pos, m_range);

            if (camDistance <= (m_range + 300.0f) && camVel.Mag() < 5.0f && camInteractive && inView)
              Trigger();

            if (camDistance <= m_range && camVel.Mag() < 5.0f && camInteractive)
              Trigger();
          }
          else
          {
            int numFound;
            int numCorrectTypeFound = 0;
            WorldObjectId* ids = g_app->m_location->m_entityGrid->GetNeighbours(m_pos.x, m_pos.z, m_range, &numFound);
            for (int i = 0; i < numFound; ++i)
            {
              WorldObjectId id = ids[i];
              if (id.IsValid() && id.GetTeamId() == m_id.GetTeamId())
              {
                Entity* entity = g_app->m_location->GetEntity(id);
                if (entity && entity->m_entityType == m_entityType)
                {
                  ++numCorrectTypeFound;
                  break;
                }
              }
            }

            if (numCorrectTypeFound > 0)
              Trigger();
          }
        }
      }
    }
  }

  return Building::Advance();
}

bool ScriptTrigger::DoesSphereHit(const LegacyVector3& _pos, float _radius) { return false; }

bool ScriptTrigger::DoesShapeHit(Shape* _shape, Matrix34 _transform) { return false; }

bool ScriptTrigger::DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, const float _rayLen,
                               LegacyVector3* _pos, LegacyVector3* _norm)
{
  return false;
}

int ScriptTrigger::GetBuildingLink() { return m_linkId; }

void ScriptTrigger::SetBuildingLink(const int _buildingId) { m_linkId = _buildingId; }

void ScriptTrigger::Read(TextReader* _in, const bool _dynamic)
{
  Building::Read(_in, _dynamic);

  m_linkId = atoi(_in->GetNextToken());
  m_range = atof(_in->GetNextToken());

  strcpy(m_scriptFilename, _in->GetNextToken());

  char* entityType = _in->GetNextToken();
  if (_stricmp(entityType, "always") == 0)
    m_entityType = SCRIPTRIGGER_RUNALWAYS;
  else if (_stricmp(entityType, "never") == 0)
    m_entityType = SCRIPTRIGGER_RUNNEVER;
  else if (_stricmp(entityType, "camenter") == 0)
    m_entityType = SCRIPTRIGGER_RUNCAMENTER;
  else if (_stricmp(entityType, "camview") == 0)
    m_entityType = SCRIPTRIGGER_RUNCAMVIEW;
  else
    m_entityType = Entity::GetTypeId(entityType);
}
