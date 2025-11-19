#pragma once

#include "entity.h"

#define CENTIPEDE_MINSEARCHRANGE        75.0f
#define CENTIPEDE_MAXSEARCHRANGE        150.0f
#define CENTIPEDE_SPIRITEATRANGE        10.0f
#define CENTIPEDE_NUMSPIRITSTOREGROW    4
#define CENTIPEDE_MAXSIZE               20

class Centipede : public Entity
{
  protected:
    float m_size;
    WorldObjectId m_next;                         // Guy infront of me
    WorldObjectId m_prev;                         // Guy behind me

    LegacyVector3 m_targetPos;
    WorldObjectId m_targetEntity;

    LList<LegacyVector3> m_positionHistory;
    bool m_linked;
    float m_panic;
    int m_numSpiritsEaten;
    float m_lastAdvance;

    static Shape* s_shapeBody;
    static Shape* s_shapeHead;

    bool SearchForRandomPosition();
    bool SearchForTargetEnemy();
    bool SearchForSpirits();
    bool SearchForRetreatPosition();

    bool AdvanceToTargetPosition();
    void RecordHistoryPosition();
    bool GetTrailPosition(LegacyVector3& _pos, LegacyVector3& _vel, int _numSteps);

    void Panic(float _time);
    void EatSpirits();

  public:
    Centipede();

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;
    void Render(float _predictionTime) override;

    bool IsInView() override;

    void Attack(const LegacyVector3& _pos) override;
};
