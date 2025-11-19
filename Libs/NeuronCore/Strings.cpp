#include "pch.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "FileSys.h"

using json = nlohmann::json;

void Strings::Load(const std::wstring& _file)
{
  std::ifstream f;

  std::wstring path = FileSys::GetHomeDirectory() + _file;
  f.open(path, std::ifstream::in);

  if (f.is_open())
  {
    json data = json::parse(f);

    for (const auto& item : data)
    {
      auto name = item.value("Key", "");
      auto value = item.value("Value", "");
      if (!name.empty() && !value.empty())
        sm_strings[name] = value;
    }
  }
}

std::string Strings::Get(std::string_view _key)
{
  auto it = sm_strings.find(std::string(_key));
  if (it != sm_strings.end())
    return it->second;
  return std::string(_key);
}
