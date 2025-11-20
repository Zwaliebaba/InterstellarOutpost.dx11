#pragma once

#ifdef PROFILER_ENABLED
#include <pix3.h>
#endif

namespace Neuron
{
#ifdef PROFILER_ENABLED
  inline void StartProfile(const std::wstring_view _itemName) { PIXBeginEvent(PIX_COLOR_DEFAULT, _itemName.data()); }
  inline void EndProfile() { PIXEndEvent(); }
  inline void SetMarker(const std::wstring_view _itemName) { PIXSetMarker(PIX_COLOR_DEFAULT, _itemName.data()); }
#else
  constexpr void StartProfile([[maybe_unused]] std::wstring_view _itemName) {}
  constexpr void EndProfile() {}
  constexpr void SetMarker([[maybe_unused]] const std::wstring_view _itemName) {}
#endif
}// namespace Neuron
