#ifndef INCLUDED_INPUT_FILE_PATHS_H
#define INCLUDED_INPUT_FILE_PATHS_H

class InputPrefs
{
  InputPrefs(); // Don't instantiate
  public:
    static const std::string& GetSystemPrefsPath();
    static const std::string& GetUserPrefsPath();
};

#endif // INCLUDED_INPUT_FILE_PATHS_H
