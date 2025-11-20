#ifndef INCLUDED_PREFERENCES_H
#define INCLUDED_PREFERENCES_H

#include "fast_darray.h"

class PrefsItem
{
  public:
    std::string m_key;

    enum
    {
      TypeString,
      TypeFloat,
      TypeInt
    };

    int m_type;
    std::string m_str;

    union
    {
      int m_int;
      float m_float;
    };

    bool m_hasBeenWritten;

    PrefsItem();
    PrefsItem(char* _line);
    PrefsItem(const char* _key, const char* _str);
    PrefsItem(const char* _key, float _float);
    PrefsItem(const char* _key, int _int);
    ~PrefsItem();
};

class PrefsManager
{
public:
  PrefsManager(const std::string& _filename);
  ~PrefsManager();

  void Load(const char* _filename = nullptr); // If filename is NULL, then m_filename is used
  void Save();

  void Clear();

  const char* GetString(const char* _key, const char* _default = nullptr) const;
  float GetFloat(const char* _key, float _default = -1.0f) const;
  int GetInt(const char* _key, int _default = -1) const;

  // Set functions update existing PrefsItems or create new ones if key doesn't yet exist
  void SetString(const char* _key, const char* _string);
  void SetFloat(const char* _key, float _val);
  void SetInt(const char* _key, int _val);

  void AddLine(const char* _line, bool _overwrite = false);

  bool DoesKeyExist(const char* _key);

  protected:
  std::unordered_map<std::string, PrefsItem*> m_items;
  std::vector<std::string> m_fileText;
  std::string m_filename;

  bool IsLineEmpty(const char* _line);
  void SaveItem(FILE* out, PrefsItem* _item);

  void CreateDefaultValues();
};

extern PrefsManager* g_prefsManager;

#endif
