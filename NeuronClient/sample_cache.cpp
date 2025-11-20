#include "pch.h"
#include "sample_cache.h"
#include "GameApp.h"
#include "resource.h"
#include "sound_stream_decoder.h"

CachedSampleManager g_cachedSampleManager;
bool g_deletingCachedSampleHandle = false;

CachedSample::CachedSample(const char* _sampleName)
  : m_amountCached(0)
{
  char fullPath[512] = "sounds/";
  strcat(fullPath, _sampleName);

  m_soundStreamDecoder = Resource::GetSoundStreamDecoder(fullPath);
  ASSERT_TEXT(m_soundStreamDecoder, "Failed to open sound stream decoder : %s", fullPath);

  m_numChannels = m_soundStreamDecoder->m_numChannels;
  m_numSamples = m_soundStreamDecoder->m_numSamples;
  m_freq = m_soundStreamDecoder->m_freq;

  m_rawSampleData = NEW signed short[m_numChannels * m_numSamples];
}

CachedSample::~CachedSample()
{
  delete m_soundStreamDecoder;
  m_soundStreamDecoder = nullptr;
  delete [] m_rawSampleData;
  m_rawSampleData = nullptr;
}

void CachedSample::Read(signed short* _data, unsigned int _startSample, unsigned int _numSamples)
{
  if (m_soundStreamDecoder)
  {
    int highestSampleRequested = _startSample + _numSamples - 1;
    if (highestSampleRequested >= m_amountCached)
    {
      int amountToRead = highestSampleRequested - m_amountCached + 1;
      unsigned int amountRead = m_soundStreamDecoder->Read(&m_rawSampleData[m_amountCached], amountToRead);
      m_amountCached += amountRead;
      DEBUG_ASSERT(m_amountCached <= m_numSamples);
      if (m_amountCached == m_numSamples)
      {
        delete m_soundStreamDecoder;
        m_soundStreamDecoder = nullptr;
      }
    }
  }

  memcpy(_data, &m_rawSampleData[_startSample], sizeof(signed short) * m_numChannels * _numSamples);
}

//*****************************************************************************
// Class CachedSampleHandle
//*****************************************************************************

CachedSampleHandle::CachedSampleHandle(CachedSample* _sample)
  : m_nextSampleIndex(0),
    m_cachedSample(_sample) {}

CachedSampleHandle::~CachedSampleHandle()
{
  m_cachedSample = nullptr;
  m_nextSampleIndex = 0xffffffff;
}

unsigned int CachedSampleHandle::Read(signed short* _data, unsigned int _numSamples)
{
  int samplesRemaining = m_cachedSample->m_numSamples - m_nextSampleIndex;
  _numSamples = std::min<unsigned int>(_numSamples, samplesRemaining);

  m_cachedSample->Read(_data, m_nextSampleIndex, _numSamples);
  m_nextSampleIndex += _numSamples;

  return _numSamples;
}

void CachedSampleHandle::Restart() { m_nextSampleIndex = 0; }

CachedSampleManager::~CachedSampleManager()
{
}

CachedSampleHandle* CachedSampleManager::GetSample(const char* _sampleName)
{
  auto it = m_cache.find(_sampleName);
	CachedSample* cachedSample = nullptr;

  if (it == m_cache.end())
  {
    cachedSample = NEW CachedSample(_sampleName);
    m_cache.emplace(_sampleName, cachedSample);
  }
  else
		cachedSample = it->second.get();

  auto rv = NEW CachedSampleHandle(cachedSample);
  return rv;
}

void CachedSampleManager::EmptyCache() { m_cache.clear(); }
