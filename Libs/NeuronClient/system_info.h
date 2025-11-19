#ifndef INCLUDED_SYSTEM_INFO_H
#define INCLUDED_SYSTEM_INFO_H

class AudioInfo
{
  public:
    char** m_deviceNames;
    unsigned int m_numDevices;
    int m_preferredDevice;

    AudioInfo()
      : m_deviceNames(nullptr),
        m_numDevices(0) {}

    ~AudioInfo()
    {
      for (unsigned int i = 0; i < m_numDevices; ++i)
        free(m_deviceNames[i]);
      delete[] m_deviceNames;
    }
};

class SystemInfo
{
  void GetAudioDetails();

  public:
    AudioInfo m_audioInfo;

    SystemInfo();
    ~SystemInfo();
};

extern SystemInfo* g_systemInfo;

#endif
