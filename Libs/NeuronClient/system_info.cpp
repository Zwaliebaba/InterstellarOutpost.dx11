#include "pch.h"
#include <mmsystem.h>
#include "system_info.h"

SystemInfo* g_systemInfo = nullptr;

SystemInfo::SystemInfo()
{
  GetAudioDetails();
}

SystemInfo::~SystemInfo() {}

void SystemInfo::GetAudioDetails()
{
  unsigned int numDevs = waveOutGetNumDevs();
  m_audioInfo.m_numDevices = numDevs;
  m_audioInfo.m_preferredDevice = -1;
  int bestScore = -1000;

  m_audioInfo.m_deviceNames = NEW char*[numDevs];
  for (unsigned int i = 0; i < numDevs; ++i)
  {
    WAVEOUTCAPSA caps;
    waveOutGetDevCapsA(i, &caps, sizeof(WAVEOUTCAPSA));
    m_audioInfo.m_deviceNames[i] = _strupr(caps.szPname);

    int score = 0;
    if (caps.dwFormats & WAVE_FORMAT_1M08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_1M16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_1S08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_1S16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2M08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2M16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2S08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2S16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4M08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4M16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4S08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4S16)
      score++;
    if (caps.dwSupport & WAVECAPS_SYNC)
      score -= 10;
    if (caps.dwSupport & WAVECAPS_LRVOLUME)
      score++;
    if (caps.dwSupport & WAVECAPS_PITCH)
      score++;
    if (caps.dwSupport & WAVECAPS_PLAYBACKRATE)
      score++;
    if (caps.dwSupport & WAVECAPS_VOLUME)
      score++;
    if (caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
      score++;

    if (score > bestScore)
      m_audioInfo.m_preferredDevice = i;
  }

  ASSERT_TEXT(m_audioInfo.m_preferredDevice != -1, "No suitable audio hardware found");
}
