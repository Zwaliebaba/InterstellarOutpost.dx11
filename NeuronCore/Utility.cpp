#include "pch.h"
#include "Utility.h"

std::vector<std::string> WordWrapText(std::string_view _string, float _lineWidth, float _fontWidth, bool _wrapToWindow)
{
  if (_string.empty())
    return {};

  if (_lineWidth < 0 && _wrapToWindow)
    return {};

  // Calculate the maximum width in characters for 1 line
  size_t linewidth = _lineWidth / _fontWidth;

  // Create a copy of the string which we will use as our output strings
  std::string newstring = std::string(_string);

  std::vector<std::string> llist;

  while (true)
  {
    size_t nextnewline = newstring.find_first_of('\n');
    if (nextnewline == std::string::npos)
      break;

    if (_wrapToWindow && nextnewline > linewidth)
    {
      // This line is too long and needs trimming down
      auto space = newstring.find_first_of(' ');

      if (space != std::string::npos)
      {
        llist.emplace_back(newstring.substr(0, space));
        newstring.erase(0, space);
      }
      else
      {
        llist.emplace_back(newstring.substr(0, linewidth));
        newstring.erase(0, linewidth);
      }
    }
    else
    {
      // Found a newline char - replace with a terminator
      // then add this position in as the next line
      // then continue from this position
      llist.emplace_back(newstring.substr(0, nextnewline));
      newstring.erase(0, nextnewline);
    }
  }

  if (!newstring.empty())
  {
    // Add the last part of the string if it exists
    llist.emplace_back(newstring);
  }
  return llist;
}
