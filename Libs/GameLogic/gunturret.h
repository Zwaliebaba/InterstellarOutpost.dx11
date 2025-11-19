#ifndef _included_gunturret_h
#define _included_gunturret_h

#include "building.h"
#include "effect.h"

#define GUNTURRET_RETARGETTIMER     3.0f
#define GUNTURRET_MINRANGE          100.0f
#define GUNTURRET_MAXRANGE          300.0f
#define GUNTURRET_NUMSTATUSMARKERS  5
#define GUNTURRET_NUMBARRELS        4
#define GUNTURRET_OWNERSHIPTIMER    1.0f

class GunTurret : public Building
{
  protected:
    Shape* m_turret;
    Shape* m_barrel;
    ShapeMarker* m_barrelMount;
    ShapeMarker* m_barrelEnd[GUNTURRET_NUMBARRELS];
    ShapeMarker* m_statusMarkers[GUNTURRET_NUMSTATUSMARKERS];

    LegacyVector3 m_turretFront;
    LegacyVector3 m_barrelUp;

    bool m_aiTargetCreated;

    LegacyVector3 m_target;
    WorldObjectId m_targetId;
    float m_fireTimer;
    int m_nextBarrel;
    float m_retargetTimer;
    bool m_targetCreated;

    float m_health;
    float m_ownershipTimer;

    void PrimaryFire();
    bool SearchForTargets();
    void SearchForRandomPos();
    void RecalculateOwnership();

  public:
    GunTurret();

    void Initialize(Building* _template) override;

    void ExplodeBody();
    void Damage(float _damage) override;
    bool Advance() override;

    LegacyVector3 GetTarget();

    bool IsInView() override;
    void Render(float _predictionTime) override;
    void RenderPorts() override;
};

class GunTurretTarget : public Effect
{
  public:
    int m_buildingId;

    GunTurretTarget(int _buildingId);
    bool Advance() override;
    void Render(float _time) override;
};

#endif
