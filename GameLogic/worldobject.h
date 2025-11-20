#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H

#include "ASyncLoader.h"
#include "LegacyVector3.h"

#define UNIT_BUILDINGS      -100
#define UNIT_SPIRITS        -101
#define UNIT_LASERS         -102
#define UNIT_EFFECTS        -103

class WorldObjectId
{
  public:
    WorldObjectId();
    WorldObjectId(unsigned char _teamId, int _unitId, int _index, int _uniqueId);
    void Set(unsigned char _teamId, int _unitId, int _index, int _uniqueId);

    void SetInvalid();

    void SetTeamId(unsigned char _teamId) { m_teamId = _teamId; }
    void SetUnitId(int _unitId) { m_unitId = _unitId; }
    void SetIndex(int _index) { m_index = _index; }
    void SetUniqueId(int _uniqueId) { m_uniqueId = _uniqueId; }
    void GenerateUniqueId();

    unsigned char GetTeamId() const { return m_teamId; }
    int GetUnitId() const { return m_unitId; }
    int GetIndex() const { return m_index; }
    int GetUniqueId() const { return m_uniqueId; }

    bool IsValid() const { return (m_teamId != 255 || m_unitId != -1 || m_index != -1); }

    bool operator !=(const WorldObjectId& w) const;
    bool operator ==(const WorldObjectId& w) const;
    const WorldObjectId& operator =(const WorldObjectId& w);

  protected:
    unsigned char m_teamId;
    int m_unitId;
    int m_index;
    int m_uniqueId;

    static int s_nextUniqueId;
};

enum class ObjectType : int8_t
{
  Invalid = -1,
  Building,
  Effect,
  Entity,
};

class WorldObject : public ASyncLoader
{
  public:
    explicit WorldObject(ObjectType _type);
    virtual ~WorldObject() = default;
    virtual fire_and_forget LoadAssets() { co_return; }

    void BounceOffLandscape();

    [[nodiscard]] ObjectType GetObjectType() const { return m_type; }
    void SetObjectType(ObjectType type) { m_type = type; }

    virtual bool Advance();
    virtual void Render(float _time);

    WorldObjectId m_id;
    LegacyVector3 m_pos;
    LegacyVector3 m_vel;
    bool m_onGround;
    bool m_enabled;

    LegacyVector3 m_centerPos;
    float m_radius;                   // Can be Zero, which means its not an object we can collide with

  protected:
    ObjectType m_type;
};

class Light
{
  public:
    float m_colour[4];	// Forth element seems irrelevant but OpenGL insists we specify it
    float m_front[4];	// Forth element must be 0.0f to signify an infinitely distance light

    Light();
    void SetColor(float colour[4]);
    void SetFront(float front[4]);
    void SetFront(LegacyVector3 front);
    void Normalize();
};

#endif
