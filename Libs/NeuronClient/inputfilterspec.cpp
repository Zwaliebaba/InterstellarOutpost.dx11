#include "pch.h"

#include "inputfilterspec.h"

unsigned long newFilterSpecID()
{
  static unsigned long nextID = 0;
  return nextID++;
}
