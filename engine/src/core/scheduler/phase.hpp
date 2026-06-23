#pragma once

#include "defines.hpp"

struct Phase {
	StringView name;

	explicit Phase(const StringView name) : name(name) {}
	explicit Phase(const char* name) : name(name) {}
};
