#pragma once

class Strings
{
	public:
		static void Load(const std::wstring& _file);
		static std::string Get(std::string_view _key);

  protected:
		inline static std::unordered_map<std::string, std::string> sm_strings;
};

