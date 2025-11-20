#ifndef _included_armyant_h
#define _included_armyant_h

#include "entity.h"
#include "shape.h"

#define ARMYANT_SEARCHRANGE         10

class ArmyAnt : public Entity
{
  protected:
    float m_scale;
    Shape* m_shapes[3];
    ShapeMarker* m_carryMarker;

    bool AdvanceScoutArea();
    bool AdvanceCollectSpirit();
    bool AdvanceCollectEntity();
    bool AdvanceAttackEnemy();
    bool AdvanceReturnToBase();
    bool AdvanceToTargetPosition();
    bool AdvanceBaseDestroyed();

    bool SearchForTargets();
    bool SearchForSpirits();
    bool SearchForEnemies();
    bool SearchForAntHill();
    bool SearchForRandomPosition();

  public:
    LegacyVector3 m_wayPoint;
    int m_orders;
    bool m_targetFound;
    int m_spiritId;
    WorldObjectId m_targetId;

    enum
    {
      NoOrders,
      ScoutArea,
      CollectSpirit,
      CollectEntity,
      AttackEnemy,
      ReturnToBase,
      BaseDestroyed
    };

    ArmyAnt();

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;
    void Render(float _predictionTime) override;

    void OrderReturnToBase();

    void GetCarryMarker(LegacyVector3& _pos, LegacyVector3& _vel);
};

#endif
