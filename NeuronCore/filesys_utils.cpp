#include "pch.h"
#include "filesys_utils.h"
#include "string_utils.h"

std::vector<std::string> ListDirectory(const char* _dir, const char* _filter, bool _fullFilename)
{
  std::vector<std::string> result;

  if (_filter == nullptr || _filter[0] == '\0')
    _filter = "*";

  // Now add on all files found locally
  char searchstring[256];
  DEBUG_ASSERT(strlen(_dir) + strlen(_filter) < sizeof(searchstring) - 1);

  sprintf(searchstring, "%s%s", _dir, _filter);

  WIN32_FIND_DATAA thisfile;
  const HANDLE hFind = FindFirstFileA(searchstring, &thisfile);
  if (hFind == INVALID_HANDLE_VALUE)
    return result;

  do
  {
    if (!(thisfile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      std::string newname;
      if (_fullFilename)
        newname = std::string(_dir) + thisfile.cFileName;
      else
        newname = thisfile.cFileName;

      result.emplace_back(newname);
    }
  }
  while (FindNextFileA(hFind, &thisfile) != 0);

  return result;
}

std::vector<std::string> ListSubDirectoryNames(const char* _dir)
{
  std::vector<std::string> result;

  WIN32_FIND_DATAA thisfile;
  const HANDLE hFind = FindFirstFileA(_dir, &thisfile);

  do
  {
    if (strcmp(thisfile.cFileName, ".") != 0 && strcmp(thisfile.cFileName, "..") != 0 && (thisfile.dwFileAttributes &
      FILE_ATTRIBUTE_DIRECTORY))
    {
      std::string newname = thisfile.cFileName;
      result.emplace_back(newname);
    }
  }
  while (FindNextFileA(hFind, &thisfile) != 0);
  return result;
}

bool DoesFileExist(const char* _fullPath)
{
  FILE* f = fopen(_fullPath, "r");
  if (f)
  {
    fclose(f);
    return true;
  }

  return false;
}

#define FILE_PATH_BUFFER_SIZE 256
static char s_filePathBuffer[FILE_PATH_BUFFER_SIZE + 1];

const char* GetDirectoryPart(const char* _fullFilePath)
{
  strncpy(s_filePathBuffer, _fullFilePath, FILE_PATH_BUFFER_SIZE);

  char* finalSlash = strrchr(s_filePathBuffer, '/');
  if (finalSlash)
  {
    *(finalSlash + 1) = '\x0';
    return s_filePathBuffer;
  }

  return nullptr;
}

const char* GetFilenamePart(const char* _fullFilePath)
{
  const char* filePart = strrchr(_fullFilePath, '/') + 1;
  if (filePart)
  {
    strncpy(s_filePathBuffer, filePart, FILE_PATH_BUFFER_SIZE);
    return s_filePathBuffer;
  }
  return nullptr;
}

const char* GetExtensionPart(const char* _fullFilePath) { return strrchr(_fullFilePath, '.') + 1; }

const char* RemoveExtension(const char* _fullFileName)
{
  strcpy(s_filePathBuffer, _fullFileName);

  char* dot = strrchr(s_filePathBuffer, '.');
  if (dot)
    *dot = '\x0';

  return s_filePathBuffer;
}
