#ifndef _included_darwinian_h
#define _included_darwinian_h

#include "Effect.h"
#include "entity.h"

#define DARWINIAN_SEARCHRANGE_OFFICERS      75.0f
#define DARWINIAN_SEARCHRANGE_ARMOUR        200.0f
#define DARWINIAN_SEARCHRANGE_SPIRITS       50.0f
#define DARWINIAN_SEARCHRANGE_THREATS       90.0f
#define DARWINIAN_SEARCHRANGE_GRENADES      60.0f
#define DARWINIAN_SEARCHRANGE_TURRETS       90.0f
#define DARWINIAN_SEARCHRANGE_PORTS         100.0f

#define DARWINIAN_FEARRANGE                 200.0f

class Darwinian : public Entity
{
public:
  Darwinian();
  fire_and_forget LoadAssets() override;

  void Begin() override;
  bool Advance(Unit* _unit) override;
  void ChangeHealth(int _amount) override;
  bool IsInView() override;
  void Render(float _predictionTime, float _highDetail);

  LegacyVector3 PushFromObstructions(const LegacyVector3& pos, bool killem = true) override;

  void GiveOrders(const LegacyVector3& _targetPos);
  void TakeControl(int _controllerId);
  void AntCapture(WorldObjectId _antId);
  void WatchSpectacle(int _buildingId);
  void CastShadow(int _buildingId);
  void BoardRocket(int _buildingId);
  void AttackBuilding(int _buildingId);
  void SetFire();
  bool IsOnFire();

    enum
    {
      StateIdle,
      StateApproachingPort,
      StateOperatingPort,
      StateApproachingArmour,
      StateInsideArmour,
      StateWorshipSpirit,
      StateUnderControl,
      StateFollowingOrders,
      StateFollowingOfficer,
      StateCombat,
      StateCapturedByAnt,
      StateBoardingRocket,
      StateWatchingSpectacle,
      StateAttackingBuilding,                        // Eg attacking rocket in demo2
      StateOnFire
    };

    int m_state;
    bool m_promoted;
    LegacyVector3 m_wayPoint;

  protected:
    std::shared_ptr<Texture> m_santaHatTex;
    std::shared_ptr<Texture> m_darTex;

    float m_retargetTimer;
    WorldObjectId m_threatId;
    WorldObjectId m_armourId;
    WorldObjectId m_officerId;
    int m_spiritId;
    int m_buildingId;
    int m_portId;
    WorldObjectId m_boxKiteId;

    float m_threatRange;
    bool m_scared;

    int m_controllerId;                         // Used only when Under
    int m_wayPointId;                           // Control
    bool m_teleportRequired;                     //

    LegacyVector3 m_orders;                               // Used when receiving orders
    int m_ordersBuildingId;                     // from an officer
    bool m_ordersSet;                            //

    float m_grenadeTimer;
    float m_officerTimer;

    int m_shadowBuildingId;         // This building causes us to cast a shadow
    LegacyVector3 m_avoidObstruction;         // Used to nagivate around big obstructions, eg water

    bool SearchForNewTask();

    bool SearchForRandomPosition();
    bool SearchForThreats();
    bool SearchForSpirits();
    bool SearchForPorts();
    bool SearchForOfficers();
    bool SearchForArmour();
    bool BeginVictoryDance();

    bool AdvanceIdle();
    bool AdvanceApproachingPort();
    bool AdvanceOperatingPort();
    bool AdvanceApproachingArmour();
    bool AdvanceInsideArmour();
    bool AdvanceWorshipSpirit();
    bool AdvanceUnderControl();
    bool AdvanceFollowingOrders();
    bool AdvanceFollowingOfficer();
    bool AdvanceCombat();
    bool AdvanceCapturedByAnt();
    bool AdvanceWatchingSpectacle();
    bool AdvanceBoardingRocket();
    bool AdvanceAttackingBuilding();
    bool AdvanceOnFire();

    bool AdvanceToTargetPosition();
};

class BoxKite : public Effect
{
  public:
    BoxKite();
    fire_and_forget LoadAssets() override;

    bool Advance() override;
    void Release();
    void Render(float _predictionTime) override;

    std::shared_ptr<Texture> m_starburst;
    Shape* m_shape;
    LegacyVector3 m_front;
    LegacyVector3 m_up;

    enum
    {
      StateHeld,
      StateReleased
    };

    int m_state;

    float m_birthTime;
    float m_deathTime;
    float m_brightness;
    float m_size;
};

#endif
