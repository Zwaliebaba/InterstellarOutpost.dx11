#pragma once

#include "worldobject.h"

enum class EffectType : int8_t
{
  Invalid = -1,
  EffectThrowableGrenade,
  EffectThrowableAirstrikeMarker,
  EffectThrowableAirstrikeBomb,
  EffectThrowableControllerGrenade,
  EffectGunTurretTarget,
  EffectGunTurretShell,
  EffectSpamInfection,
  EffectBoxKite,
  EffectSnow,
  EffectRocket,
  EffectShockwave,
  LaserBeam,
  Missile,
  EffectMuzzleFlash,
  EffectOfficerOrders,
  EffectZombie
};

class Effect : public WorldObject
{
  public:
    explicit Effect(EffectType _type)
      : WorldObject(ObjectType::Effect),
        m_effectType(_type) {}

  public:
    EffectType m_effectType;
};
