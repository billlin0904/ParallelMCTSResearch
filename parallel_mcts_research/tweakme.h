#pragma once

#define USE_PPL 0
#define USE_STD_UNORDERED_SET 0

#if USE_STD_UNORDERED_SET
#include <unordered_set>
#else
#include <parallel_hashmap/phmap.h>
#endif

#define ENABLE_JSON 1

#define ENABLE_COROUTINE 0
