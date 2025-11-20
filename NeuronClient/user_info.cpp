#include "pch.h"
#include "text_stream_readers.h"

#include <string.h>

// Parses the user id file and fills in those fields
// Which must be allocated beforehand
bool GetUserInfoData(const char* _userInfoFilename, const char** _username, const char** _email)
{
  static char* s_username = nullptr;
  static char* s_email = nullptr;

  if (!s_username || !s_email)
  {
    TextFileReader fileReader(_userInfoFilename);
    if (fileReader.IsOpen())
    {
      fileReader.ReadLine();

      ASSERT_TEXT(_stricmp( fileReader.GetNextToken(), "Username" ) == 0, "Failed to parse %s", _userInfoFilename);

      s_username = _strupr(fileReader.GetRestOfLine());

      fileReader.ReadLine();

      ASSERT_TEXT(_stricmp( fileReader.GetNextToken(), "Email" ) == 0, "Failed to parse %s", _userInfoFilename);

      s_email = _strupr(fileReader.GetRestOfLine());
    }
  }

  *_username = s_username;
  *_email = s_email;

  return (s_username && s_email);
}
