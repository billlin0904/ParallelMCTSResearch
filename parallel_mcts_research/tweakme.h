// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once

#define USE_PPL 1
#define USE_STD_UNORDERED_SET 1
#define USE_STD_UNORDERED_MAP 1

#if USE_STD_UNORDERED_SET
#include <unordered_set>
#else
#include <parallel_hashmap/phmap.h>
#endif

#if USE_STD_UNORDERED_MAP
#include <unordered_map>
#else
#include <parallel_hashmap/phmap.h>
#endif

#if USE_STD_UNORDERED_SET
template <typename T>
using HashSet = std::unordered_set<T>;
#else
template <typename T>
using HashSet = phmap::flat_hash_set<T>;
#endif

#if USE_STD_UNORDERED_MAP
template <typename Key, typename Value>
using HashMap = std::unordered_map<Key, Value>;
#else
template <typename Key, typename Value>
using HashMap = phmap::flat_hash_map<Key, Value>;
#endif

#define ENABLE_JSON 1

#define ENABLE_COROUTINE 0
