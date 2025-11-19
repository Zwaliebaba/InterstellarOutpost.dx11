#pragma once

class SoundStreamDecoder;

class CachedSample
{
  protected:
    SoundStreamDecoder* m_soundStreamDecoder;	// NULL once sample has been read fully once
    signed short* m_rawSampleData;
    unsigned int m_amountCached;				// Zero at first, ranging up to m_numSamples once sample has been read fully

  public:
    unsigned int m_numChannels;
    unsigned int m_freq;
    unsigned int m_numSamples;

    CachedSample(const char* _sampleName);
    ~CachedSample();

    void Read(signed short* _data, unsigned int _startSample, unsigned int _numSamples);
};

class CachedSampleHandle
{
  protected:
    unsigned int m_nextSampleIndex;			// Index of first sample to return when Read is called

  public:
    CachedSample* m_cachedSample;

    CachedSampleHandle(CachedSample* _sample);
    ~CachedSampleHandle();

    unsigned int Read(signed short* _data, unsigned int _numSamples);
    void Restart();
};

class CachedSampleManager
{
  protected:
    std::map<std::string, std::unique_ptr<CachedSample>> m_cache;

  public:
    ~CachedSampleManager();

    CachedSampleHandle* GetSample(const char* _sampleName);	// Delete the returned object when you are finished with it

    void EmptyCache();              // Deletes all cached sample data
};

extern CachedSampleManager g_cachedSampleManager;

extern bool g_deletingCachedSampleHandle;
