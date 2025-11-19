#ifndef _included_bridge_h
#define _included_bridge_h

#include "teleport.h"

#define BRIDGE_TRANSPORTPERIOD          0.1f
#define BRIDGE_TRANSPORTSPEED           50.0f

class Bridge : public Teleport
{
  public:
    enum
    {
      BridgeTypeEnd,
      BridgeTypeTower,
      NumBridgeTypes
    };

    int m_bridgeType;
    int m_nextBridgeId;
    float m_status;                           // Construction status, 0=not started, 100=finished, < 0.0f = shutdown

  protected:
    Shape* m_shapes[NumBridgeTypes];
    ShapeMarker* m_signal;

    bool m_beingOperated;

  public:
    Bridge();

    void Initialize(Building* _template) override;
    void SetBridgeType(int _type);

    void Render(float predictionTime) override;
    void RenderAlphas(float predictionTime) override;
    bool Advance() override;

    bool GetAvailablePosition(LegacyVector3& _pos, LegacyVector3& _front);                     // Finds place for engineer
    void BeginOperation();
    void EndOperation();

    bool ReadyToSend() override;
    LegacyVector3 GetStartPoint() override;
    LegacyVector3 GetEndPoint() override;
    bool GetExit(LegacyVector3& _pos, LegacyVector3& _front) override;

    bool UpdateEntityInTransit(Entity* _entity) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

#endif
