// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once


#include "threadpool.h"

#include <robin_hood.h>

template <typename T>
using HashSet = robin_hood::unordered_set<T>;

template <typename Key, typename Value>
using HashMap = robin_hood::unordered_map<Key, Value>;

