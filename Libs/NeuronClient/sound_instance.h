#ifndef INCLUDED_SOUND_INSTANCE_H
#define INCLUDED_SOUND_INSTANCE_H

#include "LegacyVector3.h"
#include "entity.h"
#include "sound_parameter.h"

class SoundInstance;
class SoundStreamDecoder;
class DspEffect;
class CachedSampleHandle;

// ============================================================================
// class DspHandle

#define MAX_PARAMS  13

class DspHandle
{
  public:
    int m_type;                         // enum'd in SoundLibrary3d

    SoundParameter m_params[MAX_PARAMS];
    SoundInstance* m_parent;

    DspHandle();

    void Copy(DspHandle* _copyMe);
    void Initialize(SoundInstance* _parent);
    void Advance();
};

// ============================================================================
// class SoundInstanceId

class SoundInstanceId
{
  protected:
    static int m_nextUniqueId;

  public:
    SoundInstanceId();

    int m_index;
    int m_uniqueId;

    void SetInvalid();

    bool operator ==(const SoundInstanceId& w) const;

    static int GenerateUniqueId();
};

// ============================================================================
// class SoundInstance

class SoundInstance
{
  public:
    enum                            // Position Types
    {
      Type2D,
      Type3DStationary,
      Type3DAttachedToObject,
      TypeInEditor,
      NumPositionTypes
    };

    enum                            // Instance types
    {
      Polyphonic,
      MonophonicRandom,
      MonophonicNearest,
      NumInstanceTypes
    };

    enum                            // Loop Types
    {
      SinglePlay,
      Looped,
      LoopedADSR,
      NumLoopTypes
    };

    enum                            // Source types
    {
      Sample,
      SampleGroupRandom,
      NumSourceTypes
    };

    enum                            // ADSR State types
    {
      StateAttack,
      StateDecay,
      StateSustain,
      StateRelease
    };

    SoundInstanceId m_id;

    char m_soundName[256];
    int m_positionType;
    int m_instanceType;
    int m_loopType;
    int m_sourceType;

    int m_restartAttempts;          // Used to give single play sounds more than one chance to start

    float m_minDistance;              // Distance (m) at which sound begins to attenuate
    SoundParameter m_volume;                   // Channel volume, logarithmic, 0.0f (silence) to 10.0f (full)
    SoundParameter m_attack;                   // ADSR
    SoundParameter m_sustain;                  // ADSR
    SoundParameter m_release;                  // ADSR
    int m_adsrState;                // ADSR
    float m_adsrTimer;                // ADSR
    float m_channelVolume;            // m_volume after considering ADSR
    float m_perceivedVolume;          // m_channelVolume after considering 3d location

    SoundParameter m_freq;

    SoundParameter m_loopDelay;
    float m_loopDelayTimer;
    bool m_restartOccured;

    LegacyVector3 m_pos;
    LegacyVector3 m_vel;
    LList<WorldObjectId*> m_objIds;
    WorldObjectId m_objId;                // The selected objId from the list

    float m_calculatedPriority;
    int m_channelIndex;

    CachedSampleHandle* m_cachedSampleHandle;
    SoundInstance* m_parent;                  // The blueprint from which I was copied

    LList<DspHandle*> m_dspFX;

    std::string m_eventName;

    void OpenStream(bool _keepCurrentStream);  // Handles sound groups, file types etc

    SoundInstance();
    ~SoundInstance();

    void SetSoundName(const char* _name);
    void SetEventName(const char* _entityName, const char* _eventName);

    void Copy(SoundInstance* _copyMe);
    bool StartPlaying(int _channelIndex);
    bool IsPlaying();
    bool Advance();                                     // Only call me if we are actually playing
    bool AdvanceLoop();                                     // true if loop phase is complete and we can write more samples
    void BeginRelease(bool _final);                        // Enters Release phase of sound
    void StopPlaying();                                     // Stops sound immediately (if playing)

    int GetChannelIndex();
    char* GetDescriptor();

    bool Update3DPosition();                             // Will only set the pos if required
    bool UpdateChannelVolume();                             // Takes into account ADSR. Returns true if done
    void CalculatePerceivedVolume();                             // Fills in m_perceivedVolume

    void ForceParameter(SoundParameter& _param, float value);        // Forces the instance values eg vel, pos
    bool UpdateParameter(SoundParameter& _param);                     // Returns true if any change occured

    void PropagateBlueprints();                                             // Call this to update all looping sounds

    WorldObject* GetAttachedObject();

    static const char* GetPositionTypeName(int _type);
    static const char* GetInstanceTypeName(int _type);
    static const char* GetLoopTypeName(int _type);
    static const char* GetSourceTypeName(int _type);
};

#endif
