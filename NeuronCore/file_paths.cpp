#include "pch.h"
#include "file_paths.h"

using std::string;

static const string INPUT_DATA_DIR = "input/";
static const string INPUT_PREFS_FILE = "input_preferences.txt";
static const string USER_INPUT_PREFS_FILE = "user_input_preferences.txt";

using localeIt = std::map<int, string>::const_iterator;

const string& InputPrefs::GetSystemPrefsPath()
{
  static string path = "";

  if (path == "")
    path = INPUT_DATA_DIR + INPUT_PREFS_FILE;

  return path;
}

const string& InputPrefs::GetUserPrefsPath()
{
  static string path = "";

  if (path == "")
  {
    path = USER_INPUT_PREFS_FILE;
  }

  return path;
}
