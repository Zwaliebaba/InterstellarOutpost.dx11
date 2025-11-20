#ifndef _included_spam_h
#define _included_spam_h

#include "Effect.h"
#include "building.h"

#define SPAM_RELOADTIME             60.0f
#define SPAM_DAMAGE                 100.0f

class Spam : public Building
{
  protected:
    float m_timer;
    float m_damage;

    bool m_research;
    bool m_onGround;
    bool m_activated;

  public:
    Spam();

    void Initialize(Building* _template) override;
    void Damage(float _damage) override;
    void Destroy(float _intensity) override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool Advance() override;

    void SetAsResearch();
    void SendFromHeaven();
    void SpawnInfection();
};

#define SPAMINFECTION_MINSEARCHRANGE    100.0f
#define SPAMINFECTION_MAXSEARCHRANGE    200.0f
#define SPAMINFECTION_LIFE              10.0f
#define SPAMINFECTION_TAILLENGTH        30

class SpamInfection : public Effect
{
  protected:
    enum
    {
      StateIdle,
      StateAttackingEntity,
      StateAttackingSpirit
    };

    int m_state;
    float m_retargetTimer;
    WorldObjectId m_targetId;
    int m_spiritId;
    LegacyVector3 m_targetPos;
    float m_life;

    LList<LegacyVector3> m_positionHistory;

    void AdvanceIdle();
    void AdvanceAttackingEntity();
    void AdvanceAttackingSpirit();
    bool AdvanceToTargetPosition();

    bool SearchForEntities();
    bool SearchForRandomPosition();
    bool SearchForSpirits();

  public:
    int m_parentId;                     // id of Spam that spawned me

    SpamInfection();

    bool Advance() override;
    void Render(float _time) override;
};

#endif
