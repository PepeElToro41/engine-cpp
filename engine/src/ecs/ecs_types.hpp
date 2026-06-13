#pragma once

#include "defines.hpp"

using EntityId = u64;
using ComponentId = u64;
using Id = u64;
using ArchetypeId = u64;

struct TypeInfo {
    usz length;
    usz alignment;
};