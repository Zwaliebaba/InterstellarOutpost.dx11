#ifndef INCLUDED_SOUND_LIBRARY_3D_DSOUND_H
#define INCLUDED_SOUND_LIBRARY_3D_DSOUND_H

#include "sound_library_3d.h"

class DirectSoundChannel;
class DirectSoundData;
struct IDirectSoundBuffer;

//*****************************************************************************
// Class SoundLibrary3dDirectSound
//*****************************************************************************

class SoundLibrary3dDirectSound : public SoundLibrary3d
{
  protected:
    DirectSoundChannel* m_channels;
    DirectSoundChannel* m_musicChannel;
    DirectSoundData* m_directSound;

    IDirectSoundBuffer* CreateSecondaryBuffer(int _numSamples);
    void RefreshCapabilities();
    long CalcWrappedDelta(long a, long b, unsigned long bufferSize);
    void PopulateBuffer(int _channel, int _fromSample, int _numSamples, bool _isMusic);
    void CommitChanges();					// Commits all pos/or/vel etc changes
    void AdvanceChannel(int _channel, int _frameNum);
    int GetNumFilters(int _channel);
    void Verify();

  public:
    SoundLibrary3dDirectSound();
    ~SoundLibrary3dDirectSound() override;

    void Initialize(int _mixFreq, int _numChannels, bool hw3d, int _mainBufNumSamples, int _musicBufNumSamples) override;

    bool Hardware3DSupport() override;
    int GetMaxChannels() override;
    int GetCPUOverhead() override;
    float GetChannelHealth(int _channel) override;					// 0.0 = BAD, 1.0 = GOOD
    int GetChannelBufSize(int _channel) const override;

    void ResetChannel(int _channel) override;					// Refills entire channel with data immediately

    void SetChannel3DMode(int _channel, int _mode) override;		// 0 = 3d, 1 = head relative, 2 = disabled
    void SetChannelPosition(int _channel, const LegacyVector3& _pos, const LegacyVector3& _vel) override;
    void SetChannelFrequency(int _channel, int _frequency) override;
    void SetChannelMinDistance(int _channel, float _minDistance) override;
    void SetChannelVolume(int _channel, float _volume) override;	// logarithmic, 0.0 - 10.0, 0=practially silent

    void EnableDspFX(int _channel, int _numFilters, const int* _filterTypes) override;
    void UpdateDspFX(int _channel, int _filterType, int _numParams, const float* _params) override;
    void DisableDspFX(int _channel) override;

    void SetListenerPosition(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up,
                             const LegacyVector3& _vel) override;

    void Advance() override;
};

#endif
