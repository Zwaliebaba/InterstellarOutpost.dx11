#include "pch.h"
#include "soundsystem.h"
#include "GameApp.h"
#include "btree.h"
#include "camera.h"
#include "entity.h"
#include "filesys_utils.h"
#include "location.h"
#include "main.h"
#include "math_utils.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "sample_cache.h"
#include "sound_library_2d.h"
#include "sound_library_3d.h"
#include "sound_library_3d_dsound.h"
#include "sound_stream_decoder.h"
#include "string_utils.h"
#include "text_stream_readers.h"

#pragma comment(lib, "winmm.lib")

#define SOUNDSYSTEM_UPDATEPERIOD    0.05f

//*****************************************************************************
// Class SoundEventBlueprint
//*****************************************************************************

SoundEventBlueprint::SoundEventBlueprint()
  : m_eventName(nullptr),
    m_instance(nullptr) {}

void SoundEventBlueprint::SetEventName(const char* _name)
{
  if (m_eventName)
  {
    delete [] m_eventName;
    m_eventName = nullptr;
  }

  if (_name)
    m_eventName = NewStr(_name);
}

//*****************************************************************************
// Class SoundSourceBlueprint
//*****************************************************************************

int SoundSourceBlueprint::GetSoundSoundType(const char* _name)
{
  for (int i = 0; i < NumOtherSoundSources; ++i)
    if (_stricmp(_name, GetSoundSourceName(i)) == 0)
      return i;
  return -1;
}

const char* SoundSourceBlueprint::GetSoundSourceName(int _type)
{
  const char* names[] = {
    "Laser",
    "Grenade",
    "Rocket",
    "AirStrikeBomb",
    "Spirit",
    "Ambience",
    "Music",
    "Interface"
  };
  static_assert(_countof( names) == NumOtherSoundSources, "Incorrect Sound Source");

  DEBUG_ASSERT(_type >= 0 && _type < NumOtherSoundSources);
  return names[_type];
}

//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

char* DspBlueprint::GetParameter(int _param, float* _min, float* _max, float* _default, int* _dataType)
{
  if (m_params.ValidIndex(_param))
  {
    DspParameterBlueprint* sb = m_params[_param];
    if (_min)
      *_min = sb->m_min;
    if (_max)
      *_max = sb->m_max;
    if (_default)
      *_default = sb->m_default;
    if (_dataType)
      *_dataType = sb->m_dataType;
    return sb->m_name;
  }

  return nullptr;
}

//*****************************************************************************
// Class SampleGroup
//*****************************************************************************

void SampleGroup::SetName(char* _name) { strcpy(m_name, _name); }

void SampleGroup::AddSample(char* _sample)
{
  char* sampleCopy = NewStr(_sample);
  m_samples.PutData(sampleCopy);
}

SampleGroup::~SampleGroup() { m_samples.EmptyAndDeleteArray(); }

//*****************************************************************************
// Class SoundSystem
//*****************************************************************************

SoundSystem::SoundSystem()
  : m_timeSync(0.0f),
    m_propagateBlueprints(false),
    m_mainProfiler(nullptr),
    m_eventProfiler(nullptr),
    m_quitWithoutSave(false),
    m_music(nullptr),
    m_requestedMusic(nullptr),
    m_channels(nullptr),
    m_numChannels(0)
{
#ifdef PROFILER_ENABLED
  m_eventProfiler = NEW Profiler();
  m_mainProfiler = NEW Profiler();
#endif
}

SoundSystem::~SoundSystem()
{
#ifdef PROFILER_ENABLED
  delete m_eventProfiler;
  delete m_mainProfiler;
#endif
  delete [] m_channels;

  m_sounds.EmptyAndDelete();
  m_entityBlueprints.EmptyAndDelete();
  m_buildingBlueprints.EmptyAndDelete();
  m_otherBlueprints.EmptyAndDelete();
  m_filterBlueprints.EmptyAndDelete();
  m_sampleGroups.EmptyAndDelete();

  delete g_soundLibrary3d;
  delete g_soundLibrary2d;
  g_soundLibrary3d = nullptr;
  g_soundLibrary2d = nullptr;
}

void SoundSystem::Initialize()
{
  LoadEffects();
  LoadBlueprints();

  RestartSoundLibrary();
}

void SoundSystem::RestartSoundLibrary()
{
  //
  // Shut down existing sound library

  if (m_channels)
  {
    delete [] m_channels;
    delete g_soundLibrary3d;
    delete g_soundLibrary2d;
    g_soundLibrary2d = nullptr;
    g_soundLibrary3d = nullptr;
  }

  //
  // Start up a new sound library

  int mixrate = g_prefsManager->GetInt("SoundMixFreq", 22050);
  int volume = g_prefsManager->GetInt("SoundMasterVolume", 255);
  m_numChannels = g_prefsManager->GetInt("SoundChannels", 32);
  int hw3d = g_prefsManager->GetInt("SoundHW3D", 0);
  const char* libName = g_prefsManager->GetString("SoundLibrary", "dsound");
  int bufSize = 20000;

  g_soundLibrary2d = NEW SoundLibrary2d;
  g_soundLibrary3d = nullptr;

  if (_stricmp(libName, "dsound") == 0)
    g_soundLibrary3d = NEW SoundLibrary3dDirectSound();

  g_soundLibrary3d->SetMasterVolume(volume);
  g_soundLibrary3d->Initialize(mixrate, m_numChannels, hw3d, bufSize, bufSize * 10);

  m_numChannels = g_soundLibrary3d->GetNumMainChannels();
  m_channels = NEW SoundInstanceId[m_numChannels];

  g_soundLibrary3d->SetMainCallback(&SoundLibraryMainCallback);
  g_soundLibrary3d->SetMusicCallback(&SoundLibraryMusicCallback);
}

void SoundSystem::StopAllDSPEffects()
{
  for (int i = 0; i < m_sounds.Size(); ++i)
  {
    if (m_sounds.ValidIndex(i))
    {
      SoundInstance* instance = m_sounds[i];
      while (instance->m_dspFX.ValidIndex(0))
      {
        DspHandle* handle = instance->m_dspFX[0];
        instance->m_dspFX.RemoveData(0);
        delete handle;
      }
    }
  }
}

bool SoundSystem::SoundLibraryMainCallback(unsigned int _channel, signed short* _data, unsigned int _numSamples,
                                           int* _silenceRemaining)
{
  if (!g_app->m_soundSystem)
    return false;

  SoundInstanceId soundId = g_app->m_soundSystem->m_channels[_channel];
  SoundInstance* instance = g_app->m_soundSystem->GetSoundInstance(soundId);

  if (instance && instance->m_cachedSampleHandle)
  {
#ifdef PROFILER_ENABLED
    g_app->m_soundSystem->m_eventProfiler->StartProfile(instance->m_eventName.c_str());
#endif

    //
    // Fill the space with sample data

    int numSamplesWritten = instance->m_cachedSampleHandle->Read(_data, _numSamples);

    if (numSamplesWritten < _numSamples)
    {
      signed short* loopStart = _data + numSamplesWritten;
      unsigned int numSamplesRemaining = _numSamples - numSamplesWritten;

      if (instance->m_loopType == SoundInstance::Looped || instance->m_loopType == SoundInstance::LoopedADSR)
      {
        while (numSamplesRemaining > 0)
        {
          bool looped = instance->AdvanceLoop();
          if (looped)
          {
            unsigned int numWritten = instance->m_cachedSampleHandle->Read(loopStart, numSamplesRemaining);
            loopStart += numWritten;
            numSamplesRemaining -= numWritten;
          }
          else
          {
            g_soundLibrary3d->WriteSilence(loopStart, numSamplesRemaining);
            numSamplesRemaining = 0;
          }
        }
      }
      else if (instance->m_loopType == SoundInstance::SinglePlay)
      {
        if (numSamplesWritten > 0)
        {
          // The sound just came to an end, so write a whole buffers worth of silence
          g_soundLibrary3d->WriteSilence(loopStart, numSamplesRemaining);
          *_silenceRemaining = g_soundLibrary3d->GetChannelBufSize(_channel) - numSamplesRemaining;
        }
        else
        {
          // The sound came to an end and now we are writing silence
          g_soundLibrary3d->WriteSilence(loopStart, numSamplesRemaining);
          *_silenceRemaining -= numSamplesRemaining;
          if (*_silenceRemaining <= 0)
            instance->BeginRelease(false);
        }
      }
    }

#ifdef PROFILER_ENABLED
    g_app->m_soundSystem->m_eventProfiler->EndProfile(instance->m_eventName.c_str());
#endif
    return true;
  }
  g_soundLibrary3d->WriteSilence(_data, _numSamples);
  return false;
}

bool SoundSystem::SoundLibraryMusicCallback(signed short* _data, unsigned int _numSamples, int* _silenceRemaining)
{
  if (!g_app->m_soundSystem)
    return false;

  SoundInstance* instance = g_app->m_soundSystem->m_music;

  if (instance && instance->m_cachedSampleHandle)
  {
#ifdef PROFILER_ENABLED
    g_app->m_soundSystem->m_eventProfiler->StartProfile(instance->m_eventName.c_str());
#endif

    //
    // Fill the space with sample data

    int numSamplesWritten = instance->m_cachedSampleHandle->Read(_data, _numSamples);

    if (numSamplesWritten < _numSamples)
    {
      signed short* loopStart = _data + numSamplesWritten;
      unsigned int numSamplesRemaining = _numSamples - numSamplesWritten;

      if (instance->m_loopType == SoundInstance::Looped || instance->m_loopType == SoundInstance::LoopedADSR)
      {
        while (numSamplesRemaining > 0)
        {
          bool looped = instance->AdvanceLoop();
          if (looped)
          {
            unsigned int numWritten = instance->m_cachedSampleHandle->Read(loopStart, numSamplesRemaining);
            loopStart += numWritten;
            numSamplesRemaining -= numWritten;
          }
          else
          {
            g_soundLibrary3d->WriteSilence(loopStart, numSamplesRemaining);
            numSamplesRemaining = 0;
          }
        }
      }
      else if (instance->m_loopType == SoundInstance::SinglePlay)
      {
        if (numSamplesWritten > 0)
        {
          // The sound just came to an end, so write a whole buffers worth of silence
          g_soundLibrary3d->WriteSilence(loopStart, numSamplesRemaining);
          *_silenceRemaining = g_soundLibrary3d->GetChannelBufSize(g_soundLibrary3d->m_musicChannelId) - numSamplesRemaining;
        }
        else
        {
          // The sound came to an end and now we are writing silence
          g_soundLibrary3d->WriteSilence(loopStart, numSamplesRemaining);
          *_silenceRemaining -= numSamplesRemaining;
          if (*_silenceRemaining <= 0)
            instance->BeginRelease(false);
        }
      }
    }

#ifdef PROFILER_ENABLED
    g_app->m_soundSystem->m_eventProfiler->EndProfile(instance->m_eventName.c_str());
#endif
    return true;
  }
  g_soundLibrary3d->WriteSilence(_data, _numSamples);
  return false;
}

void SoundSystem::LoadEffects()
{
  m_filterBlueprints.SetSize(SoundLibrary3d::NUM_FILTERS);

  TextReader* in = Resource::GetTextReader("effects.txt");
  ASSERT_TEXT(in && in->IsOpen(), "Couldn't load effects.txt");

  while (in->ReadLine())
  {
    if (!in->TokenAvailable())
      continue;
    char* effect = in->GetNextToken();
    DEBUG_ASSERT(_stricmp( effect, "EFFECT" ) == 0);

    auto bp = NEW DspBlueprint();
    m_filterBlueprints.PutData(bp);
    strcpy(bp->m_name, in->GetNextToken());

    in->ReadLine();
    char* param = in->GetNextToken();
    while (_stricmp(param, "END") != 0)
    {
      auto sb = NEW DspParameterBlueprint();
      bp->m_params.PutData(sb);

      strcpy(sb->m_name, param);
      sb->m_min = atof(in->GetNextToken());
      sb->m_max = atof(in->GetNextToken());
      sb->m_default = atof(in->GetNextToken());
      sb->m_dataType = atoi(in->GetNextToken());

      in->ReadLine();
      param = in->GetNextToken();
    }
  }

  delete in;
}

void SoundSystem::LoadBlueprints()
{
  m_entityBlueprints.SetSize(SizeOfEntityType());
  m_buildingBlueprints.SetSize(SizeOfBuildingType());
  m_otherBlueprints.SetSize(SoundSourceBlueprint::NumOtherSoundSources);

  TextReader* in = Resource::GetTextReader("sounds.txt");
  ASSERT_TEXT(in && in->IsOpen(), "Couldn't open sounds.txt");

  char objectName[128];

  while (in->ReadLine())
  {
    if (!in->TokenAvailable())
      continue;
    char* group = in->GetNextToken();
    char* type = in->GetNextToken();
    bool event = false;
    SoundSourceBlueprint* ssb = nullptr;

    if (_stricmp(group, "ENTITY") == 0)
    {
      strncpy(objectName, type, 127);
      auto entityType = Entity::GetTypeId(type);
      DEBUG_ASSERT(!m_entityBlueprints.ValidIndex( I(entityType )));

      ssb = NEW SoundSourceBlueprint();
      m_entityBlueprints.PutData(ssb, I(entityType));
      event = true;
    }
    else if (_stricmp(group, "BUILDING") == 0)
    {
      strncpy(objectName, type, 127);
      auto buildingType = Building::GetTypeId(type);
      DEBUG_ASSERT(!m_buildingBlueprints.ValidIndex( I(buildingType )));

      ssb = NEW SoundSourceBlueprint();
      m_buildingBlueprints.PutData(ssb, I(buildingType));
      event = true;
    }
    else if (_stricmp(group, "OTHER") == 0)
    {
      strncpy(objectName, type, 127);
      int otherType = SoundSourceBlueprint::GetSoundSoundType(type);
      DEBUG_ASSERT(otherType >= 0 && otherType < SoundSourceBlueprint::NumOtherSoundSources);
      DEBUG_ASSERT(!m_otherBlueprints.ValidIndex( otherType ));

      ssb = NEW SoundSourceBlueprint();
      m_otherBlueprints.PutData(ssb, otherType);
      event = true;
    }
    else if (_stricmp(group, "SAMPLEGROUP") == 0)
    {
      strncpy(objectName, "sample group", 127);
      SampleGroup* sampleGroup = NewSampleGroup(type);
      ParseSampleGroup(in, sampleGroup);
    }

    if (event)
    {
      while (in->ReadLine())
      {
        if (in->TokenAvailable())
        {
          char* word = in->GetNextToken();
          if (_stricmp(word, "END") == 0)
            break;
          DEBUG_ASSERT(_stricmp( word, "EVENT" ) == 0);
          ParseSoundEvent(in, ssb, objectName);
        }
      }
    }
  }

  //
  // Fill in the non-specified sound sources with blanks

  for (EntityType i : RangeEntityType())
  {
    if (!m_entityBlueprints.ValidIndex(I(i)))
    {
      auto ssb = NEW SoundSourceBlueprint();
      m_entityBlueprints.PutData(ssb, I(i));
    }
  }

  for (BuildingType i : RangeBuildingType())
  {
    if (!m_buildingBlueprints.ValidIndex(I(i)))
    {
      auto ssb = NEW SoundSourceBlueprint();
      m_buildingBlueprints.PutData(ssb, I(i));
    }
  }

  for (int i = 0; i < SoundSourceBlueprint::NumOtherSoundSources; ++i)
  {
    if (!m_otherBlueprints.ValidIndex(i))
    {
      auto ssb = NEW SoundSourceBlueprint();
      m_otherBlueprints.PutData(ssb, i);
    }
  }

  delete in;
}

void SoundSystem::ParseSoundEvent(TextReader* _in, SoundSourceBlueprint* _source, const char* _entityName)
{
  auto seb = NEW SoundEventBlueprint();
  seb->SetEventName(_in->GetNextToken());

  seb->m_instance = NEW SoundInstance();
  seb->m_instance->SetEventName(_entityName, seb->m_eventName);

  int oldUserPriority;            // backwards compatability, no longer used

  _in->ReadLine();
  char* fieldName = _in->GetNextToken();
  while (_stricmp(fieldName, "END") != 0)
  {
    if (_stricmp(fieldName, "SOUNDNAME") == 0)
    {
      char* soundName = _in->GetNextToken();
      StrToLower(soundName);
      const char* extensionRemoved = RemoveExtension(soundName);
      seb->m_instance->SetSoundName(extensionRemoved);
    }
    else if (_stricmp(fieldName, "SOURCETYPE") == 0)
      seb->m_instance->m_sourceType = atoi(_in->GetNextToken());
    else if (_stricmp(fieldName, "POSITIONTYPE") == 0)
      seb->m_instance->m_positionType = atoi(_in->GetNextToken());
    else if (_stricmp(fieldName, "INSTANCETYPE") == 0)
      seb->m_instance->m_instanceType = atoi(_in->GetNextToken());
    else if (_stricmp(fieldName, "LOOPTYPE") == 0)
      seb->m_instance->m_loopType = atoi(_in->GetNextToken());
    else if (_stricmp(fieldName, "PRIORITY") == 0)
      oldUserPriority = atoi(_in->GetNextToken());
    else if (_stricmp(fieldName, "MINDISTANCE") == 0)
      seb->m_instance->m_minDistance = atof(_in->GetNextToken());
    else if (_stricmp(fieldName, "VOLUME") == 0)
      seb->m_instance->m_volume.Read(_in);
    else if (_stricmp(fieldName, "FREQUENCY") == 0)
      seb->m_instance->m_freq.Read(_in);
    else if (_stricmp(fieldName, "ATTACK") == 0)
      seb->m_instance->m_attack.Read(_in);
    else if (_stricmp(fieldName, "SUSTAIN") == 0)
      seb->m_instance->m_sustain.Read(_in);
    else if (_stricmp(fieldName, "RELEASE") == 0)
      seb->m_instance->m_release.Read(_in);
    else if (_stricmp(fieldName, "LOOPDELAY") == 0)
      seb->m_instance->m_loopDelay.Read(_in);
    else if (_stricmp(fieldName, "EFFECT") == 0)
      ParseSoundEffect(_in, seb);
    else
      DEBUG_ASSERT(false);

    // This is bad, we have a looping sound that won't be attached
    // to any one object
    //        DEBUG_ASSERT( !( seb->m_instance->m_loopType &&
    //                        seb->m_instance->m_positionType != SoundInstance::Type3DAttachedToObject ) );

    _in->ReadLine();
    fieldName = _in->GetNextToken();
  }

  _source->m_events.PutData(seb);
}

void SoundSystem::ParseSoundEffect(TextReader* _in, SoundEventBlueprint* _blueprint)
{
  char* effectName = _in->GetNextToken();
  int fxType = -1;

  for (int i = 0; i < m_filterBlueprints.Size(); ++i)
  {
    DspBlueprint* seb = m_filterBlueprints[i];
    if (_stricmp(seb->m_name, effectName) == 0)
    {
      fxType = i;
      break;
    }
  }

  DEBUG_ASSERT(fxType != -1);

  auto effect = NEW DspHandle();
  effect->m_type = fxType;

  int paramIndex = 0;
  while (true)
  {
    _in->ReadLine();
    char* paramName = _in->GetNextToken();
    if (_stricmp(paramName, "END") == 0)
      break;
    effect->m_params[paramIndex].Read(_in);
    ++paramIndex;
  }
  _blueprint->m_instance->m_dspFX.PutData(effect);
}

void SoundSystem::ParseSampleGroup(TextReader* _in, SampleGroup* _group)
{
  while (true)
  {
    _in->ReadLine();
    char* paramType = _in->GetNextToken();
    if (_stricmp(paramType, "END") == 0)
      break;

    char* sample = _in->GetNextToken();
    StrToLower(sample);
    auto extensionRemoved = (char*)RemoveExtension(sample);
    _group->AddSample(extensionRemoved);
  }
}

bool SoundSystem::InitialiseSound(SoundInstance* _instance)
{
  bool createNewSound = true;

  if (_instance->m_instanceType != SoundInstance::Polyphonic)
  {
    // This is a monophonic sound, so look for an exisiting
    // instance of the same sound
    for (int i = 0; i < m_sounds.Size(); ++i)
    {
      if (m_sounds.ValidIndex(i))
      {
        SoundInstance* thisInstance = m_sounds[i];
        if (thisInstance->m_instanceType != SoundInstance::Polyphonic && _stricmp(thisInstance->m_eventName.c_str(), _instance->m_eventName.c_str())
          == 0)
        {
          for (int j = 0; j < _instance->m_objIds.Size(); ++j)
          {
            WorldObjectId* id = _instance->m_objIds[j];
            thisInstance->m_objIds.PutData(NEW WorldObjectId(*id));
          }
          createNewSound = false;
          break;
        }
      }
    }
  }

  if (createNewSound)
  {
    _instance->m_id.m_index = m_sounds.PutData(_instance);
    _instance->m_id.m_uniqueId = SoundInstanceId::GenerateUniqueId();
    _instance->m_restartAttempts = 3;           //int( 1.0f + (float) _instance->m_volume.GetOutput() / 5.0f );
    return true;
  }
  return false;
}

int SoundSystem::FindBestAvailableChannel()
{
  int emptyChannelIndex = -1;                                         // Channel without a current sound or requested sound

  int lowestPriorityChannelIndex = -1;                                // Channel with very low priority
  float lowestPriorityChannel = 999999.9f;

  for (int i = 0; i < m_numChannels; ++i)
  {
    SoundInstanceId soundId = m_channels[i];
    SoundInstance* currentSound = GetSoundInstance(soundId);

    float channelPriority = 0.0f;
    if (currentSound)
      channelPriority = currentSound->m_calculatedPriority;
    else
    {
      emptyChannelIndex = i;
      break;
    }

    if (channelPriority < lowestPriorityChannel)
    {
      lowestPriorityChannel = channelPriority;
      lowestPriorityChannelIndex = i;
    }
  }

  //
  // Did we find any empty channels?

  if (emptyChannelIndex != -1)
    return emptyChannelIndex;
  return lowestPriorityChannelIndex;
}

void SoundSystem::ShutdownSound(SoundInstance* _instance)
{
  if (m_sounds.ValidIndex(_instance->m_id.m_index))
    m_sounds.MarkNotUsed(_instance->m_id.m_index);

  _instance->StopPlaying();
  delete _instance;
}

SoundInstance* SoundSystem::GetSoundInstance(SoundInstanceId id)
{
  if (!m_sounds.ValidIndex(id.m_index))
    return nullptr;

  SoundInstance* found = m_sounds[id.m_index];
  if (found->m_id == id)
    return found;

  return nullptr;
}

void SoundSystem::TriggerEntityEvent(Entity* _entity, const char* _eventName)
{
  if (!m_channels)
    return;

  START_PROFILE(m_mainProfiler, "TriggerEntityEvent");
  WorldObjectId objId(_entity->m_id);

  if (m_entityBlueprints.ValidIndex(I(_entity->m_entityType)))
  {
    SoundSourceBlueprint* sourceBlueprint = m_entityBlueprints[I(_entity->m_entityType)];
    for (int i = 0; i < sourceBlueprint->m_events.Size(); ++i)
    {
      SoundEventBlueprint* seb = sourceBlueprint->m_events[i];
      if (_stricmp(seb->m_eventName, _eventName) == 0)
      {
        DEBUG_ASSERT(seb->m_instance);
        auto instance = NEW SoundInstance();
        instance->Copy(seb->m_instance);
        instance->m_objIds.PutData(NEW WorldObjectId(objId));
        instance->m_pos = _entity->m_pos;
        instance->m_vel = _entity->m_vel;
        bool success = InitialiseSound(instance);
        if (!success)
          ShutdownSound(instance);
      }
    }
  }

  END_PROFILE(m_mainProfiler, "TriggerEntityEvent");
}

void SoundSystem::TriggerBuildingEvent(Building* _building, const char* _eventName)
{
  if (!m_channels)
    return;

  START_PROFILE(m_mainProfiler, "TriggerBuildingEvent");
  if (m_buildingBlueprints.ValidIndex(I(_building->m_buildingType)))
  {
    SoundSourceBlueprint* sourceBlueprint = m_buildingBlueprints[I(_building->m_buildingType)];
    for (int i = 0; i < sourceBlueprint->m_events.Size(); ++i)
    {
      SoundEventBlueprint* seb = sourceBlueprint->m_events[i];
      if (_stricmp(seb->m_eventName, _eventName) == 0)
      {
        DEBUG_ASSERT(seb->m_instance);
        auto instance = NEW SoundInstance();
        instance->Copy(seb->m_instance);
        instance->m_objIds.PutData(NEW WorldObjectId(_building->m_id));
        instance->m_pos = _building->m_pos;
        bool success = InitialiseSound(instance);
        if (!success)
          ShutdownSound(instance);
      }
    }
  }

  END_PROFILE(m_mainProfiler, "TriggerBuildingEvent");
}

void SoundSystem::TriggerOtherEvent(WorldObject* _other, const char* _eventName, int _type)
{
  if (!m_channels)
    return;

  START_PROFILE(m_mainProfiler, "TriggerOtherEvent");

  static int musicType = -1;
  if (musicType == -1)
    musicType = SoundSourceBlueprint::GetSoundSoundType("music");

  if (m_otherBlueprints.ValidIndex(_type))
  {
    // Search for the event blueprint matching _eventName
    SoundSourceBlueprint* sourceBlueprint = m_otherBlueprints[_type];
    for (int i = 0; i < sourceBlueprint->m_events.Size(); ++i)
    {
      SoundEventBlueprint* seb = sourceBlueprint->m_events[i];
      if (_stricmp(seb->m_eventName, _eventName) == 0)
      {
        // We have a match
        DEBUG_ASSERT(seb->m_instance);
        auto instance = NEW SoundInstance();
        instance->Copy(seb->m_instance);
        if (_type == musicType)
        {
          //if( m_music && _stricmp( m_music->m_eventName+6, _eventName ) == 0 )
          if (m_music && _stricmp(m_music->m_soundName, seb->m_instance->m_soundName) == 0)
          {
            // The music is already playing
          }
          else
          {
            m_requestedMusic = instance;
            if (m_music)
              m_music->BeginRelease(true);
          }
        }
        else
        {
          if (_other)
          {
            instance->m_pos = _other->m_pos;
            instance->m_objIds.PutData(NEW WorldObjectId(_other->m_id));
          }
          bool success = InitialiseSound(instance);
          if (!success)
            ShutdownSound(instance);
        }
      }
    }
  }

  END_PROFILE(m_mainProfiler, "TriggerOtherEvent");
}

void SoundSystem::TriggerDuplicateSound(SoundInstance* _instance)
{
  auto newInstance = NEW SoundInstance();
  newInstance->Copy(_instance);
  newInstance->m_parent = _instance->m_parent;
  newInstance->m_pos = _instance->m_pos;
  newInstance->m_vel = _instance->m_vel;

  for (int i = 0; i < _instance->m_objIds.Size(); ++i)
  {
    WorldObjectId* id = _instance->m_objIds[i];
    newInstance->m_objIds.PutData(NEW WorldObjectId(*id));
  }

  bool success = InitialiseSound(newInstance);
  if (success && newInstance->m_positionType == SoundInstance::TypeInEditor)
    m_editorInstanceId = newInstance->m_id;
  else if (!success)
    ShutdownSound(newInstance);
}

void SoundSystem::StopAllSounds(WorldObjectId _id, const char* _eventName)
{
  if (strstr(_eventName, "Music"))
  {
    if (m_music)
      m_music->BeginRelease(true);
  }
  else
  {
    for (int i = 0; i < m_sounds.Size(); ++i)
    {
      if (m_sounds.ValidIndex(i))
      {
        SoundInstance* instance = m_sounds[i];

        if (instance->m_objId == _id)
        {
          if (!_eventName || _stricmp(instance->m_eventName.c_str(), _eventName) == 0)
          {
            if (instance->IsPlaying())
              instance->BeginRelease(true);
            else
              ShutdownSound(instance);
          }
        }
      }
    }
  }
}

int SoundSystem::IsSoundPlaying(SoundInstanceId _id)
{
  for (int i = 0; i < m_numChannels; ++i)
  {
    SoundInstanceId soundId = m_channels[i];
    if (_id == soundId)
      return i;
  }

  return -1;
}

int SoundSystem::NumInstancesPlaying(WorldObjectId _id, char* _eventName)
{
  int result = 0;

  for (int i = 0; i < m_numChannels; ++i)
  {
    SoundInstanceId soundId = m_channels[i];
    SoundInstance* instance = GetSoundInstance(soundId);
    bool instanceMatch = !_id.IsValid() || instance->m_objId == _id;

    if (instance && instanceMatch && _stricmp(instance->m_eventName.c_str(), _eventName) == 0)
      ++result;
  }

  return result;
}

int SoundSystem::NumInstances(WorldObjectId _id, const char* _eventName)
{
  int result = 0;

  for (int i = 0; i < m_sounds.Size(); ++i)
  {
    if (m_sounds.ValidIndex(i))
    {
      SoundInstance* instance = m_sounds[i];
      bool instanceMatch = !_id.IsValid() || instance->m_objId == _id;

      if (instance && instanceMatch && _stricmp(instance->m_eventName.c_str(), _eventName) == 0)
        ++result;
    }
  }

  return result;
}

int SoundSystem::NumSoundInstances() { return m_sounds.NumUsed(); }

int SoundSystem::NumChannelsUsed()
{
  int numUsed = 0;
  for (int i = 0; i < m_numChannels; ++i)
  {
    SoundInstanceId id = m_channels[i];
    if (GetSoundInstance(id) != nullptr)
      numUsed++;
  }

  return numUsed;
}

int SoundSystem::NumSoundsDiscarded() { return NumSoundInstances() - NumChannelsUsed(); }

int SoundInstanceCompare(const void* elem1, const void* elem2)
{
  SoundInstanceId id1 = *((SoundInstanceId*)elem1);
  SoundInstanceId id2 = *((SoundInstanceId*)elem2);

  SoundInstance* instance1 = g_app->m_soundSystem->GetSoundInstance(id1);
  SoundInstance* instance2 = g_app->m_soundSystem->GetSoundInstance(id2);

  DEBUG_ASSERT(instance1);
  DEBUG_ASSERT(instance2);

  if (instance1->m_perceivedVolume < instance2->m_perceivedVolume)
    return +1;
  if (instance1->m_perceivedVolume > instance2->m_perceivedVolume)
    return -1;
  return 0;
}

void SoundSystem::Advance()
{
  if (!m_channels)
    return;

#ifdef PROFILER_ENABLED
  m_mainProfiler->Advance();
  m_eventProfiler->Advance();
#endif

  m_timeSync += g_advanceTime;
  if (m_timeSync >= SOUNDSYSTEM_UPDATEPERIOD)
  {
    m_timeSync -= SOUNDSYSTEM_UPDATEPERIOD;

    START_PROFILE(g_app->m_profiler, "Advance SoundSystem");

    //
    // Advance music

    if (m_music)
    {
      bool amIDone = m_music->Advance();
      if (amIDone)
      {
        START_PROFILE(g_app->m_profiler, "Shutdown Music");
        ShutdownSound(m_music);
        m_music = nullptr;
        END_PROFILE(g_app->m_profiler, "Shutdown Music");
      }
    }

    if (m_music == nullptr && m_requestedMusic != nullptr)
    {
      m_music = m_requestedMusic;
      m_requestedMusic = nullptr;
      m_music->OpenStream(false);
      m_music->m_channelIndex = g_soundLibrary3d->m_musicChannelId;
      g_soundLibrary3d->ResetChannel(m_music->m_channelIndex);
      g_soundLibrary3d->SetChannelFrequency(m_music->m_channelIndex, m_music->m_cachedSampleHandle->m_cachedSample->m_freq);
    }

    //
    // Resync with blueprints (changed by editor)

    START_PROFILE(g_app->m_profiler, "Propagate Blueprints");
    if (m_propagateBlueprints)
      PropagateBlueprints();
    END_PROFILE(g_app->m_profiler, "Propagate Blueprints");

    //
    // First pass : Recalculate all Perceived Sound Volumes
    // Throw away sounds that have had their chance
    // Build a list of instanceIDs for sorting

    START_PROFILE(g_app->m_profiler, "Allocate Sorted Array");
    static int sortedArraySize = 128;
    static SoundInstanceId* sortedIds = nullptr;
    if (m_sounds.NumUsed() > sortedArraySize)
    {
      delete [] sortedIds;
      sortedIds = nullptr;
      while (sortedArraySize < m_sounds.NumUsed())
        sortedArraySize *= 2;
    }
    if (!sortedIds)
      sortedIds = NEW SoundInstanceId[sortedArraySize];

    int numSortedIds = 0;
    END_PROFILE(g_app->m_profiler, "Allocate Sorted Array");

    START_PROFILE(g_app->m_profiler, "Perceived Volumes");
    for (int i = 0; i < m_sounds.Size(); ++i)
    {
      if (m_sounds.ValidIndex(i))
      {
        SoundInstance* instance = m_sounds[i];
        if (!instance->IsPlaying() && !instance->m_loopType)
          instance->m_restartAttempts--;
        if (instance->m_restartAttempts < 0)
          ShutdownSound(instance);
        else if (instance->m_positionType == SoundInstance::Type3DAttachedToObject && !instance->GetAttachedObject())
          ShutdownSound(instance);
        else
        {
          instance->CalculatePerceivedVolume();
          sortedIds[numSortedIds] = instance->m_id;
          numSortedIds++;
        }
      }
    }
    END_PROFILE(g_app->m_profiler, "Perceived Volumes");

    //
    // Sort sounds into perceived volume order
    // NOTE : There are exactly numSortedId elements in sortedIds.
    // NOTE : It isn't safe to assume numSortedIds == m_sounds.NumUsed()

    START_PROFILE(g_app->m_profiler, "Sort Samples");
    qsort(sortedIds, numSortedIds, sizeof(SoundInstanceId), SoundInstanceCompare);
    END_PROFILE(g_app->m_profiler, "Sort Samples");

    //
    // Second pass : Recalculate all Sound Priorities starting with the nearest sounds
    // Reduce priorities as more of the same sounds are played

    BTree<float> existingInstances;

    //
    // Also look out for the highest priority new sound to swap in

    START_PROFILE(g_app->m_profiler, "Recalculate Priorities");

    SoundInstance* newInstance = nullptr;
    float highestInstancePriority = 0.0f;

    for (int i = 0; i < numSortedIds; ++i)
    {
      SoundInstanceId id = sortedIds[i];
      SoundInstance* instance = GetSoundInstance(id);
      DEBUG_ASSERT(instance);

      instance->m_calculatedPriority = instance->m_perceivedVolume;

      if (BTree<float>* existingInstance = existingInstances.LookupTree(instance->m_eventName.c_str()))
      {
        instance->m_calculatedPriority *= existingInstance->data;
        existingInstance->data *= 0.75f;
      }
      else
        existingInstances.PutData(instance->m_eventName.c_str(), 0.75f);

      if (!instance->IsPlaying() && instance->m_calculatedPriority > highestInstancePriority)
      {
        newInstance = instance;
        highestInstancePriority = instance->m_calculatedPriority;
      }
    }

    END_PROFILE(g_app->m_profiler, "Recalculate Priorities");

    if (newInstance)
    {
      // Find worst old sound to get rid of
      START_PROFILE(g_app->m_profiler, "Find best Channel");
      int bestAvailableChannel = FindBestAvailableChannel();
      END_PROFILE(g_app->m_profiler, "Find best Channel");

      START_PROFILE(g_app->m_profiler, "Stop Old Sound");
      // Stop the old sound
      SoundInstance* existingInstance = GetSoundInstance(m_channels[bestAvailableChannel]);
      if (existingInstance && !existingInstance->m_loopType)
        ShutdownSound(existingInstance);
      else if (existingInstance)
        existingInstance->StopPlaying();
      END_PROFILE(g_app->m_profiler, "Stop Old Sound");

      START_PROFILE(g_app->m_profiler, "Start New Sound");
      // Start the new sound
      bool success = newInstance->StartPlaying(bestAvailableChannel);
      if (success)
        m_channels[bestAvailableChannel] = newInstance->m_id;
      else
      {
        // This is fairly bad, the sound failed to play
        // Which means it failed to load, or to go into a channel
        ShutdownSound(newInstance);
      }
      END_PROFILE(g_app->m_profiler, "Start New Sound");

      START_PROFILE(g_app->m_profiler, "Reset Channel");
      g_soundLibrary3d->ResetChannel(bestAvailableChannel);
      END_PROFILE(g_app->m_profiler, "Reset Channel");
    }

    //
    // Advance all sound channels

    START_PROFILE(g_app->m_profiler, "Advance All Channels");
    for (int i = 0; i < m_numChannels; ++i)
    {
      SoundInstanceId soundId = m_channels[i];
      SoundInstance* currentSound = GetSoundInstance(soundId);
      if (currentSound)
      {
        bool amIDone = currentSound->Advance();
        if (amIDone)
        {
          START_PROFILE(g_app->m_profiler, "Shutdown Sound");
          ShutdownSound(currentSound);
          END_PROFILE(g_app->m_profiler, "Shutdown Sound");
        }
      }
    }
    END_PROFILE(g_app->m_profiler, "Advance All Channels");

    //
    // Update our listener position

    START_PROFILE(g_app->m_profiler, "UpdateListener");

    LegacyVector3 camUp = g_app->m_camera->GetUp();
    if (g_prefsManager->GetInt("SoundSwapStereo", 0) == 0)
      camUp *= -1.0f;

    LegacyVector3 camVel = g_app->m_camera->GetVel() * 0.2f;
    g_soundLibrary3d->SetListenerPosition(g_app->m_camera->GetPos(), g_app->m_camera->GetFront(), camUp, camVel);

    END_PROFILE(g_app->m_profiler, "UpdateListener");

    //
    // Advance our sound library

    START_PROFILE(g_app->m_profiler, "SoundLibrary3d Advance");
    g_soundLibrary3d->Advance();
    END_PROFILE(g_app->m_profiler, "SoundLibrary3d Advance");

#ifdef SOUNDSYSTEM_VERIFY
        RuntimeVerify();
#endif

    END_PROFILE(g_app->m_profiler, "Advance SoundSystem");
  }
}

void SoundSystem::RuntimeVerify()
{
  //
  // Make sure all sounds that believe they are playing have an opened sound stream

  for (int i = 0; i < m_numChannels; ++i)
  {
    SoundInstanceId id1 = m_channels[i];
    SoundInstance* currentSound = GetSoundInstance(id1);
    if (currentSound && !currentSound->m_cachedSampleHandle)
      int b = 10;
  }
}

void SoundSystem::LoadtimeVerify()
{
  // If you want to comment this out then comment out the call.

  //
  // Verify that the samples referred to in SampleGroups are valid

  FILE* soundErrors = fopen("sounderrors.txt", "wt");
  bool errorFound = false;

  for (int i = 0; i < m_sampleGroups.Size(); ++i)
  {
    if (!m_sampleGroups.ValidIndex(i))
      continue;

    SampleGroup* sg = m_sampleGroups[i];
    int size = sg->m_samples.Size();
    for (int j = 0; j < size; ++j)
    {
      char* soundName = sg->m_samples[j];
      const char* err = IsSoundSourceOK(soundName);
      if (err != nullptr)
      {
        fprintf(soundErrors, "%s: %s In sound group %s\n", err, soundName, sg->m_name);
        errorFound = true;
      }
    }
  }

  //
  // Verify that the samples referred to in SoundEventBlueprints are valid

  // Entities
  for (int i = 0; i < m_entityBlueprints.Size(); ++i)
  {
    if (!m_entityBlueprints.ValidIndex(i))
      continue;

    SoundSourceBlueprint* ssb = m_entityBlueprints[i];
    int size = ssb->m_events.Size();

    for (int j = 0; j < size; ++j)
    {
      SoundEventBlueprint* seb = ssb->m_events[j];
      SoundInstance* si = seb->m_instance;
      if (si->m_sourceType == SoundInstance::Sample)
      {
        const char* err = IsSoundSourceOK(si->m_soundName);
        if (err != nullptr)
        {
          fprintf(soundErrors, "%s: %s In entity %s, event %s\n", err, si->m_soundName, Entity::GetTypeName((EntityType)i), seb->m_eventName);
          errorFound = true;
        }
      }
      else
      {
        // Just need to verify that the group exists here
        SampleGroup* sg = GetSampleGroup(si->m_soundName);
        if (!sg)
        {
          fprintf(soundErrors, "Sound Group %s does not exist. " "Referenced by entity %s, event %s\n", si->m_soundName,
                  Entity::GetTypeName((EntityType)i), seb->m_eventName);
          errorFound = true;
        }
      }
    }
  }

  // Buildings
  for (int i = 0; i < m_buildingBlueprints.Size(); ++i)
  {
    if (!m_buildingBlueprints.ValidIndex(i))
      continue;

    SoundSourceBlueprint* ssb = m_buildingBlueprints[i];
    int size = ssb->m_events.Size();

    for (int j = 0; j < size; ++j)
    {
      SoundEventBlueprint* seb = ssb->m_events[j];
      SoundInstance* si = seb->m_instance;
      if (si->m_sourceType == SoundInstance::Sample)
      {
        const char* err = IsSoundSourceOK(si->m_soundName);
        if (err != nullptr)
        {
          fprintf(soundErrors, "%s: %s In building %s, event %s\n", err, si->m_soundName, Building::GetTypeName((BuildingType)i),
                  seb->m_eventName);
          errorFound = true;
        }
      }
      else
      {
        // Just need to verify that the group exists here
        SampleGroup* sg = GetSampleGroup(si->m_soundName);
        if (!sg)
        {
          fprintf(soundErrors, "Sound Group %s does not exist. " "Referenced by building %s, event %s\n", si->m_soundName,
                  Building::GetTypeName((BuildingType)i), seb->m_eventName);
          errorFound = true;
        }
      }
    }
  }

  // Others
  for (int i = 0; i < m_otherBlueprints.Size(); ++i)
  {
    if (!m_otherBlueprints.ValidIndex(i))
      continue;

    SoundSourceBlueprint* ssb = m_otherBlueprints[i];
    int size = ssb->m_events.Size();

    for (int j = 0; j < size; ++j)
    {
      SoundEventBlueprint* seb = ssb->m_events[j];
      SoundInstance* si = seb->m_instance;
      if (si->m_sourceType == SoundInstance::Sample)
      {
        const char* err = IsSoundSourceOK(si->m_soundName);
        if (err != nullptr)
        {
          fprintf(soundErrors, "%s: %s In Others %s, event %s\n", err, si->m_soundName, ssb->GetSoundSourceName(i),
                  seb->m_eventName);
          errorFound = true;
        }
      }
      else
      {
        // Just need to verify that the group exists here
        SampleGroup* sg = GetSampleGroup(si->m_soundName);
        if (!sg)
        {
          fprintf(soundErrors, "Sound Group %s does not exist. " "Referenced by \"others\" %s, event %s\n", si->m_soundName,
                  ssb->GetSoundSourceName(i), seb->m_eventName);
          errorFound = true;
        }
      }
    }
  }

  fclose(soundErrors);
  ASSERT_TEXT(!errorFound, "Errors found in sounds.txt : refer to sounderrors.txt for details");
}

static const char* g_soundSourceErrors[SoundSystem::SoundSourceNumErrors] = {
  nullptr,
  "Sound sample not mono",
  "Sound sample does not exist",
  "Sound sample name contains spaces"
};

const char* SoundSystem::IsSoundSourceOK(const char* _soundName)
{
#ifdef _DEBUG
  if (strchr(_soundName, ' '))
  {
    // File name contains a space
    return g_soundSourceErrors[SoundSourceFilenameHasSpace];
  }

  char fullPath[256] = "sounds/";
  strcat(fullPath, _soundName);

  SoundStreamDecoder* sound = Resource::GetSoundStreamDecoder(fullPath);
  if (!sound)
  {
    // File does not exist
    return g_soundSourceErrors[SoundSourceFileDoesNotExist];
  }

  if (sound->m_numChannels != 1)
  {
    // File isn't mono
    return g_soundSourceErrors[SoundSourceNotMono];
  }
  delete sound;
#endif

  // Everything is dandy
  return g_soundSourceErrors[SoundSourceNoError];
}

bool SoundSystem::IsSampleUsed(const char* _soundName)
{
  //
  // Entity blueprints

  for (int i = 0; i < m_entityBlueprints.Size(); ++i)
  {
    if (m_entityBlueprints.ValidIndex(i))
    {
      SoundSourceBlueprint* sourceBlueprint = m_entityBlueprints[i];
      for (int j = 0; j < sourceBlueprint->m_events.Size(); ++j)
      {
        SoundEventBlueprint* eventBlueprint = sourceBlueprint->m_events[j];
        if (eventBlueprint->m_instance)
        {
          SoundInstance* instance = eventBlueprint->m_instance;
          if (instance->m_sourceType == SoundInstance::Sample && _stricmp(instance->m_soundName, _soundName) == 0)
            return true;
          if (instance->m_sourceType != SoundInstance::Sample)
          {
            SampleGroup* group = GetSampleGroup(instance->m_soundName);
            if (group)
            {
              for (int k = 0; k < group->m_samples.Size(); ++k)
              {
                char* thisSample = group->m_samples[k];
                if (_stricmp(thisSample, _soundName) == 0)
                  return true;
              }
            }
          }
        }
      }
    }
  }

  //
  // Building blueprints

  for (int i = 0; i < m_buildingBlueprints.Size(); ++i)
  {
    if (m_buildingBlueprints.ValidIndex(i))
    {
      SoundSourceBlueprint* sourceBlueprint = m_buildingBlueprints[i];
      for (int j = 0; j < sourceBlueprint->m_events.Size(); ++j)
      {
        SoundEventBlueprint* eventBlueprint = sourceBlueprint->m_events[j];
        if (eventBlueprint->m_instance)
        {
          SoundInstance* instance = eventBlueprint->m_instance;
          if (instance->m_sourceType == SoundInstance::Sample && _stricmp(instance->m_soundName, _soundName) == 0)
            return true;
          if (instance->m_sourceType != SoundInstance::Sample)
          {
            SampleGroup* group = GetSampleGroup(instance->m_soundName);
            if (group)
            {
              for (int k = 0; k < group->m_samples.Size(); ++k)
              {
                char* thisSample = group->m_samples[k];
                if (_stricmp(thisSample, _soundName) == 0)
                  return true;
              }
            }
          }
        }
      }
    }
  }

  //
  // Other blueprints

  for (int i = 0; i < m_otherBlueprints.Size(); ++i)
  {
    if (m_otherBlueprints.ValidIndex(i))
    {
      SoundSourceBlueprint* sourceBlueprint = m_otherBlueprints[i];
      for (int j = 0; j < sourceBlueprint->m_events.Size(); ++j)
      {
        SoundEventBlueprint* eventBlueprint = sourceBlueprint->m_events[j];
        if (eventBlueprint->m_instance)
        {
          SoundInstance* instance = eventBlueprint->m_instance;
          if (instance->m_sourceType == SoundInstance::Sample && _stricmp(instance->m_soundName, _soundName) == 0)
            return true;
          if (instance->m_sourceType != SoundInstance::Sample)
          {
            SampleGroup* group = GetSampleGroup(instance->m_soundName);
            if (group)
            {
              for (int k = 0; k < group->m_samples.Size(); ++k)
              {
                char* thisSample = group->m_samples[k];
                if (_stricmp(thisSample, _soundName) == 0)
                  return true;
              }
            }
          }
        }
      }
    }
  }

  return false;
}

void SoundSystem::PropagateBlueprints()
{
  for (int i = 0; i < m_sounds.Size(); ++i)
  {
    if (m_sounds.ValidIndex(i))
    {
      SoundInstance* instance = m_sounds[i];
      instance->PropagateBlueprints();
    }
  }
}

SampleGroup* SoundSystem::GetSampleGroup(char* _name)
{
  for (int i = 0; i < m_sampleGroups.Size(); ++i)
  {
    if (m_sampleGroups.ValidIndex(i))
    {
      SampleGroup* group = m_sampleGroups[i];
      if (strcmp(group->m_name, _name) == 0)
        return group;
    }
  }

  return nullptr;
}

SampleGroup* SoundSystem::NewSampleGroup(char* _name)
{
  auto group = NEW SampleGroup();
  m_sampleGroups.PutData(group);

  if (_name)
  {
    group->SetName(_name);
    return group;
  }
  int i = 1;
  while (true)
  {
    char nameCandidate[256];
    sprintf(nameCandidate, "newsamplegroup%d", i);
    if (!GetSampleGroup(nameCandidate))
    {
      group->SetName(nameCandidate);
      return group;
    }
    ++i;
  }
}
