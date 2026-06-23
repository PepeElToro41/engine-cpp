#pragma once

#include "defines.hpp"

struct Pipeline {
	StringView name;
	
	explicit Pipeline(const StringView name) : name(name) {}
	explicit Pipeline(const char* name) : name(name) {}
};