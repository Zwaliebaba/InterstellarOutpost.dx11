#ifndef _included_souldestroyer_h
#define _included_souldestroyer_h

#include "Effect.h"
#include "entity.h"
#include "fast_darray.h"
#include "shape.h"

#define SOULDESTROYER_MINSEARCHRANGE       200.0f
#define SOULDESTROYER_MAXSEARCHRANGE       300.0f
#define SOULDESTROYER_DAMAGERANGE          25.0f
#define SOULDESTROYER_MAXSPIRITS           50

class Shape;

class SoulDestroyer : public Entity
{
  protected:
    LegacyVector3 m_targetPos;
    LegacyVector3 m_up;
    WorldObjectId m_targetEntity;
    LList<LegacyVector3> m_positionHistory;
    FastDArray<float> m_spirits;

    float m_retargetTimer;
    float m_panic;

    static Shape* s_shapeHead;
    static Shape* s_shapeTail;
    static ShapeMarker* s_tailMarker;

    LegacyVector3 m_spiritPosition[SOULDESTROYER_MAXSPIRITS];

    bool SearchForRandomPosition();
    bool SearchForTargetEnemy();
    bool SearchForRetreatPosition();

    bool AdvanceToTargetPosition();
    void RecordHistoryPosition();
    bool GetTrailPosition(LegacyVector3& _pos, LegacyVector3& _vel);

    void RenderShapes(float _predictionTime);
    void RenderSpirit(const LegacyVector3& _pos, float _alpha);

    void Panic(float _time);

  public:
    SoulDestroyer();

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;
    void Render(float _predictionTime) override;

    void Attack(const LegacyVector3& _pos) override;

    void SetWaypoint(LegacyVector3 _waypoint) override;
};

class Zombie : public Effect
{
  public:
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    float m_life;

    LegacyVector3 m_hover;
    float m_positionOffset;                       // Used to make them float around a bit
    float m_xaxisRate;
    float m_yaxisRate;
    float m_zaxisRate;

    Zombie();

    bool Advance() override;
    void Render(float _predictionTime) override;
};

#endif
