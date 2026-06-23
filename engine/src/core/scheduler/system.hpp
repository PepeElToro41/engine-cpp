#pragma once

#include "defines.hpp"
#include <functional>

struct EngineApp;
struct EngineContext;
struct Phase;

using SystemCallback = void(EngineApp* app, EngineContext* context);

struct System {
	u32 system_id;
	StringView name;
	Phase* phase = nullptr;
	Pipeline* pipeline = nullptr;
	
	std::function<SystemCallback> callback;
	
	static System create(const char* name, Phase* phase, std::function<SystemCallback> callback);
	static System create(StringView name, Phase* phase, std::function<SystemCallback> callback);
	
	static System create(const char* name, Pipeline* pipeline, std::function<SystemCallback> callback);
	static System create(StringView name, Pipeline* pipeline, std::function<SystemCallback> callback);
};