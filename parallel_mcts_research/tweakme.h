// Copyright (c) 2019 ParallelMCTSResearch project.

#pragma once


#include "threadpool.h"
#include <unordered_set>
#include <unordered_map>

template <typename T>
using HashSet = std::unordered_set<T>;

template <typename Key, typename Value>
using HashMap = std::unordered_map<Key, Value>;


