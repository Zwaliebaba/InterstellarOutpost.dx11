#pragma once

#include "entity.h"
#include "flag.h"

#define ARMOUR_UNLOADPERIOD   0.1f

class Armour : public Entity
{
  protected:
    ShapeMarker* m_markerEntrance;
    ShapeMarker* m_markerFlag;
    LegacyVector3 m_up;
    LegacyVector3 m_conversionPoint;
    float m_speed;

    enum
    {
      StateIdle,
      StateUnloading,
      StateLoading,
    };

    int m_numPassengers;
    float m_previousUnloadTimer;
    float m_newOrdersTimer;

    Flag m_flag;
    Flag m_deployFlag;

  public:
    LegacyVector3 m_wayPoint;
    int m_state;

    Armour();
    ~Armour() override;

    void Begin() override;
    bool Advance(Unit* _unit) override;
    void Render(float _predictionTime) override;

    void ChangeHealth(int _amount) override;

    void SetOrders(const LegacyVector3& _orders);
    void SetWayPoint(const LegacyVector3& _wayPoint);
    void SetConversionPoint(const LegacyVector3& _conversionPoint);
    void AdvanceToTargetPos();
    void DetectCollisions();
    void ConvertToGunTurret();

    void SetDirectOrders(); // orders while in direct control mode - removes gun turret mode

    bool IsLoading();
    bool IsUnloading();
    void ToggleLoading();
    void AddPassenger();
    void RemovePassenger();

    int Capacity();

    void GetEntrance(LegacyVector3& _exitPos, LegacyVector3& _exitDir);
};
