#ifndef _included_weapons_h
#define _included_weapons_h

#include "Effect.h"
#include "rgb_colour.h"
#include "shape.h"
#include "worldobject.h"

class ThrowableWeapon : public Effect
{
  protected:
    Shape* m_shape;
    float m_birthTime;
    float m_force;

    LegacyVector3 m_front;
    LegacyVector3 m_up;

    int m_numFlashes;

    RGBAColor m_colour;

    void TriggerSoundEvent(const char* _event);

  public:
    ThrowableWeapon(EffectType _type, const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);

    void Initialize();
    bool Advance() override;
    void Render(float _predictionTime) override;

    static float GetMaxForce(int _researchLevel);
    static float GetApproxMaxRange(float _maxForce);
};

class Grenade : public ThrowableWeapon
{
  public:
    Grenade(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);
    bool Advance() override;

    float m_life;
    float m_power;
};

class AirStrikeMarker : public ThrowableWeapon
{
  public:
    WorldObjectId m_airstrikeUnit;

    AirStrikeMarker(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);
    bool Advance() override;
};

class ControllerGrenade : public ThrowableWeapon
{
  public:
    ControllerGrenade(const LegacyVector3& _startPos, const LegacyVector3& _front, float _force);
    bool Advance() override;
};

class Rocket : public Effect
{
  public:
    unsigned char m_fromTeamId;

    Shape* m_shape;
    float m_timer;

    LegacyVector3 m_target;

    Rocket()
      : Effect(EffectType::EffectRocket)
    {}
    Rocket(LegacyVector3 _startPos, LegacyVector3 _targetPos);

    void Initialize();
    bool Advance() override;
    void Render(float predictionTime) override;
};

class Laser : public Effect
{
  public:
    Laser()
      : Effect(EffectType::LaserBeam)
    {}

    void Initialize(float _lifeTime);
    bool Advance() override;
    void Render(float predictionTime) override;

  public:
    unsigned char m_fromTeamId = {};

  protected:
    float m_life = 0.0f;
    bool m_harmless = false;                 // becomes true after hitting someone
    bool m_bounced = false;
};

class TurretShell : public Effect
{
  protected:
    float m_life;

  public:
    TurretShell(float _life);

    bool Advance() override;
    void Render(float predictionTime) override;
};

class Shockwave : public Effect
{
  public:
    Shape* m_shape;
    int m_teamId;
    float m_size;
    float m_life;

    Shockwave(int _teamId, float _size);

    bool Advance() override;
    void Render(float predictionTime) override;
};

class MuzzleFlash : public Effect
{
  public:
    LegacyVector3 m_front;
    float m_size;
    float m_life;

    MuzzleFlash();
    MuzzleFlash(const LegacyVector3& _pos, const LegacyVector3& _front, float _size, float _life);

    bool Advance() override;
    void Render(float _predictionTime) override;
};

class Missile : public Effect
{
  protected:
    float m_life;
    LList<LegacyVector3> m_history;
    Shape* m_shape;
    ShapeMarker* m_booster;
    MuzzleFlash m_fire;

  public:
    WorldObjectId m_tankId;                   // Who fired me
    LegacyVector3 m_front;
    LegacyVector3 m_up;
    LegacyVector3 m_target;

    Missile();

    bool Advance() override;
    bool AdvanceToTargetPosition(const LegacyVector3& _pos);
    void Explode();
    void Render(float _predictionTime) override;
};

#endif
