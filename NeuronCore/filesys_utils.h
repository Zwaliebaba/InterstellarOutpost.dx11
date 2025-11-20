#ifndef INCLUDED_FILESYS_UTILS
#define INCLUDED_FILESYS_UTILS

std::vector<std::string> ListDirectory(const char* _dir, const char* _filter, bool fullFilename = true);
std::vector<std::string> ListSubDirectoryNames(const char* _dir);

bool DoesFileExist(const char* _fullPath);

const char* GetDirectoryPart(const char* _fullFilePath);
const char* GetFilenamePart(const char* _fullFilePath);
const char* GetExtensionPart(const char* _fileFilePath);
const char* RemoveExtension(const char* _fullFileName);

#endif
