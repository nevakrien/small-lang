#pragma once

#include<cassert>

#define TODO assert(0 && "TODO");
#if defined(NDEBUG)
#  define UNREACHABLE() __builtin_unreachable()
#else
#  include <cassert>
#  define UNREACHABLE() do { assert(false && "Unreachable code!"); __builtin_unreachable(); } while(0)
#endif

