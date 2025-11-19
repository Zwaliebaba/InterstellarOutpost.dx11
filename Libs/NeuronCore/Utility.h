#pragma once

std::vector<std::string> WordWrapText(std::string_view _string, float _lineWidth, float _fontWidth, bool _wrapToWindow);

inline std::string StrUpr(std::string_view _string)
{
  std::string result{ _string };
  std::ranges::transform(result, result.begin(), toupper);
  return result;
}
