#ifndef _included_scripttrigger_h
#define _included_scripttrigger_h

#include "building.h"
#include "entity.h"

constexpr EntityType SCRIPTRIGGER_RUNALWAYS = static_cast<EntityType>(INT8_MAX);
constexpr EntityType SCRIPTRIGGER_RUNNEVER = static_cast<EntityType>(INT8_MAX - 1);
constexpr EntityType SCRIPTRIGGER_RUNCAMENTER = static_cast<EntityType>(INT8_MAX - 2);
constexpr EntityType SCRIPTRIGGER_RUNCAMVIEW = static_cast<EntityType>(INT8_MAX - 3);

class ScriptTrigger : public Building
{
public:
  char m_scriptFilename[256];
  float m_range;
  EntityType m_entityType;
  int m_linkId;

protected:
  int m_triggered;
  float m_timer;

public:
  ScriptTrigger();

  void Initialize(Building* _template) override;
  bool Advance() override;

  void Trigger();

  bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
  bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
  bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10,
    LegacyVector3* _pos = nullptr, LegacyVector3* _norm = nullptr) override;

  int GetBuildingLink() override;
  void SetBuildingLink(int _buildingId) override;

  void Read(TextReader* _in, bool _dynamic) override;

};

#endif
