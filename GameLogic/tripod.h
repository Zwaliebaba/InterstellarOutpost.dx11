#ifndef INCLUDED_TRIPOD_H
#define INCLUDED_TRIPOD_H

#include "LegacyVector2.h"

#include "entity.h"

class Unit;
class EntityLeg;
class Tripod;

//*****************************************************************************
// Class TripodNavData
//*****************************************************************************

class TripodNavData
{
  public:
    LegacyVector2 m_directions[6];
    int m_dir;							// Index into m_directions
    LegacyVector2 m_targetPos;

    TripodNavData();
};

//*****************************************************************************
// Class Tripod
//*****************************************************************************

class Tripod : public Entity
{
  public:
    enum
    {
      ModeWalking,
      ModePreAttack,
      ModeAttacking,
      ModePostAttack
    };

    int m_mode;

  protected:
    EntityLeg* m_legs[3];
    unsigned int m_nextLegToMove;	// Not certain to be true - just used to influence the DesireToMove score
    float m_speed;
    float m_targetHoverHeight;
    LegacyVector3 m_up;
    TripodNavData m_navData;
    LegacyVector3 m_bodyVel;
    LegacyVector3 m_attackTarget;
    float m_modeStartTime;

    void ChangeHealth(int _amount) override;

    int CalcWhichFootToMove();
    LegacyVector3 CalcAttackUpVector();
    void DoFallForTwoLegs();
    LegacyVector2 ChooseDestination();
    void DoNavigation();
    WorldObjectId FindEntityToAttack();

    void AdvanceWalk();
    void AdvancePreAttack();
    void AdvanceAttack();
    void AdvancePostAttack();

  public:
    Tripod();
    ~Tripod() override;

    bool Advance(Unit* _unit) override;
    void Render(float _predictionTime) override;
    void Begin() override;
};

#endif
