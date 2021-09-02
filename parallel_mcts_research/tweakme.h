// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once


#include "threadpool.h"

#include <unordered_map>
#include <unordered_set>
#include <robin_hood.h>

#ifdef _DEBUG
template <typename T>
using HashSet = std::unordered_set<T>;

template <typename Key, typename Value>
using HashMap = std::unordered_map<Key, Value>;
#else
template <typename T>
using HashSet = robin_hood::unordered_set<T>;

template <typename Key, typename Value>
using HashMap = robin_hood::unordered_map<Key, Value>;
#endif
