#ifndef INCLUDED_SPIDER_H
#define INCLUDED_SPIDER_H

#include "entity.h"
#include "shape.h"

class Unit;
class EntityLeg;

#define SPIDER_NUM_LEGS	8

class SpiderParameters
{
  public:
    float m_legLift;
    float m_idealLegSlope;
    float m_legSwingDuration;
    float m_delayBetweenLifts;		// Time between successive foot lifts
    float m_lookAheadCoef;			// Amount of velocity to add on to foot home pos to calc foot target pos
    float m_idealSpeed;				// Speed at which this set of parameters is appropriate
};

class Spider : public Entity
{
  public:
    enum
    {
      StateIdle,
      StateEggLaying,
      StateAttack,
      StatePouncing
    };

    int m_state;

  protected:
    SpiderParameters m_parameters[3];
    ShapeMarker* m_eggLay;

    EntityLeg* m_legs[SPIDER_NUM_LEGS];
    float
    m_nextLegMoveTime;			// Actually just the next opportunity for a leg to move - there is no guarantee that a leg will move then
    float m_delayBetweenLifts;

    float m_speed;
    float m_targetHoverHeight;
    LegacyVector3 m_targetPos;
    LegacyVector3 m_up;

    float m_pounceStartTime;

    int CalcWhichFootToMove();
    void StompFoot(const LegacyVector3& _pos);
    void UpdateLegsPouncing();
    void UpdateLegs();
    float IsPathOK(const LegacyVector3& _dest);		// Returns amount of path that can be followed
    void DetectCollisions();

    // AI stuff

    float m_retargetTimer;
    LegacyVector3 m_pounceTarget;
    int m_spiritId;

    bool SearchForRandomPos();
    bool SearchForEnemies();
    bool SearchForSpirits();

    bool AdvanceIdle();
    bool AdvanceEggLaying();
    bool AdvanceAttack();
    bool AdvancePouncing();

    bool FaceTarget();
    bool AdvanceToTarget();

  public:
    Spider();
    ~Spider() override;

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void ChangeHealth(int _amount) override;
    void Render(float _predictionTime) override;

    bool IsInView() override;
};

#endif
