#ifndef _included_mine_h
#define _included_mine_h

#include "building.h"

class Refinery;
class MineCart;

class MineBuilding : public Building
{
  protected:
    int m_trackLink;
    ShapeMarker* m_trackMarker1;
    ShapeMarker* m_trackMarker2;

    Matrix34 m_trackMatrix1;
    Matrix34 m_trackMatrix2;

    LList<MineCart*> m_carts;

    float m_previousMineSpeed;
    float m_wheelRotate;

    static Shape* s_wheelShape;
    static Shape* s_cartShape;
    static ShapeMarker* s_cartMarker1;
    static ShapeMarker* s_cartMarker2;
    static ShapeMarker* s_cartContents[3];
    static Shape* s_polygon1;
    static Shape* s_primitive1;

    static float s_refineryPopulation;
    static float s_refineryRecalculateTimer;
    static float RefinerySpeed();

  public:
    MineBuilding(BuildingType _type);

    void Initialize(Building* _template) override;
    bool Advance() override;

    bool IsInView() override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;
    void RenderCart(MineCart* _cart, float _predictionTime);

    LegacyVector3 GetTrackMarker1();
    LegacyVector3 GetTrackMarker2();

    virtual void TriggerCart(MineCart* _cart, float _initValue);

    void Read(TextReader* _in, bool _dynamic) override;
    

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;
};

class MineCart
{
  public:
    float m_progress;                     // Progress down current line, 0.0f - 1.0f

    bool m_polygons[3];
    bool m_primitives[3];

    MineCart();
};

// ****************************************************************************
// Class TrackLink
// ****************************************************************************

class TrackLink : public MineBuilding
{
  public:
    TrackLink();

    bool Advance() override;
};

// ****************************************************************************
// Class TrackJunction
// ****************************************************************************

class TrackJunction : public MineBuilding
{
  public:
    LList<int> m_trackLinks;

    TrackJunction();

    void Initialize(Building* _template) override;

    void Render(float _predictionTime) override;
    void TriggerCart(MineCart* _cart, float _initValue) override;

    void SetBuildingLink(int _buildingId) override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

// ****************************************************************************
// Class TrackStart
// ****************************************************************************

class TrackStart : public MineBuilding
{
  public:
    int m_reqBuildingId;                   // This building must be online

    TrackStart();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

// ****************************************************************************
// Class TrackEnd
// ****************************************************************************

class TrackEnd : public MineBuilding
{
  public:
    int m_reqBuildingId;                   // This building must be online

    TrackEnd();

    void Initialize(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    
};

// ****************************************************************************
// Class Refinery
// ****************************************************************************

class Refinery : public MineBuilding
{
  protected:
    ShapeMarker* m_wheel1;
    ShapeMarker* m_wheel2;
    ShapeMarker* m_wheel3;
    ShapeMarker* m_counter1;

  public:
    Refinery();

    bool Advance() override;
    void Render(float _predictionTime) override;

    const char* GetObjectiveCounter() override;

    void TriggerCart(MineCart* _cart, float _initValue) override;
};

class Mine : public MineBuilding
{
  protected:
    ShapeMarker* m_wheel1;
    ShapeMarker* m_wheel2;

  public:
    Mine();

    void Render(float _predictionTime) override;

    void TriggerCart(MineCart* _cart, float _initValue) override;
};

#endif
