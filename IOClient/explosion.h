#pragma once

#include "matrix33.h"
#include "rgb_colour.h"
#include "shape.h"

// This class maintains a rotation matrix that is used to make an ExplodingTri
// tumble as it flys through space. The m_rotMat matrix has a little bit of
// rotation multiplied on to it every frame.
class Tumbler
{
  public:
    Matrix33 m_rotMat;
    LegacyVector3 m_angVel;

    Tumbler();
    void Advance();
};

class ExplodingTri
{
  public:
    LegacyVector3 m_vel;
    LegacyVector3 m_pos;
    LegacyVector3 m_v1, m_v2, m_v3;
    LegacyVector3 m_norm;
    Tumbler* m_tumbler;
    float m_timeToDie;
    RGBAColor m_colour;
};

class Explosion
{
  protected:
    Tumbler* m_tumblers;
    unsigned int m_numTumblers;
    ExplodingTri* m_tris;
    unsigned int m_numTris;
    float m_timeToDie;

  public:
    Explosion(ShapeFragment* _frag, const Matrix34& _transform, float _fraction);
    ~Explosion();

    bool Advance();	// Returns true if the explosion has finished
    void Render();

    LegacyVector3 GetCenter() const;
};

class ExplosionManager
{
  protected:
    LList<Explosion*> m_explosions;

  public:
    ExplosionManager();
    ~ExplosionManager();

    void AddExplosion(ShapeFragment* _frag, const Matrix34& _transform, bool _recurse, float _fraction);
    void AddExplosion(Shape* _shape, const Matrix34& _transform, float _fraction = 1.0f);
    void Reset();

    void Advance();
    void Render();
};

extern ExplosionManager g_explosionManager;
