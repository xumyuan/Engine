#pragma once

#include <chrono>
#include <spdlog/sinks/basic_file_sink.h>

#define PROFILE(name) Profile __profile(name);

struct Profile
{
	Profile(const std::string& name);
	~Profile();

	std::string name;
	std::chrono::high_resolution_clock::time_point start;
	
	static std::shared_ptr<spdlog::logger> logger;
	static void initLogger();
};
