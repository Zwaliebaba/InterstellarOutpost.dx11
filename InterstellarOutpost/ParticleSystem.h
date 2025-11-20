#ifndef _included_particle_system_h
#define _included_particle_system_h

#include "LegacyVector3.h"
#include "rgb_colour.h"
#include "slice_darray.h"

// ****************************************************************************
// ParticleType
// ****************************************************************************

class ParticleType
{
  public:
    float m_spin;			// Rate of spin in radians per second (positive is clockwise)
    float m_fadeInTime;
    float m_fadeOutTime;
    float m_life;
    float m_size;
    float m_friction;		// Amount of friction to apply (0=none 1=a huge amount)
    float m_gravity;		// Amount of gravity to apply (0=none 1=quite a lot)
    RGBAColor m_colour1;		// Start of colour range
    RGBAColor m_colour2;		// End of colour range

    ParticleType();
};

// ****************************************************************************
// Particle
// ****************************************************************************

class Particle
{
  public:
    enum
    {
      TypeInvalid     = -1,
      TypeRocketTrail = 0,
      TypeExplosionCore,
      TypeExplosionDebris,
      TypeMuzzleFlash,
      TypeFire,
      TypeControlFlash,
      TypeSpark,
      TypeBlueSpark,
      TypeBrass,
      TypeMissileTrail,
      TypeMissileFire,
      TypeDarwinianFire,
      TypeLeaf,
      TypeNumTypes
    };

  private:
    static ParticleType m_types[TypeNumTypes];

  public:
    LegacyVector3 m_pos;
    LegacyVector3 m_vel;
    float m_birthTime;
    int m_typeId;
    float m_size;
    RGBAColor m_colour;

    Particle();

    void Initialize(const LegacyVector3& _pos, const LegacyVector3& _vel, int _type, float _size = -1.0f);
    bool Advance();
    void Render(float _predictionTime);

    static void SetupParticles();
};

// ****************************************************************************
// ParticleSystem
// ****************************************************************************

class ParticleSystem
{
  SliceDArray<Particle> m_particles;

  public:
    ParticleSystem();

    void CreateParticle(const LegacyVector3& _pos, const LegacyVector3& _vel, int _particleTypeId, float _size = -1.0f,
                        RGBAColor col = NULL);

    void Advance(int _slice);
    void Render();
    void Empty();
};

#endif
