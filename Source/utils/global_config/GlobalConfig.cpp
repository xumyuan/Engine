#include "pch.h"
#include "GlobalConfig.h"

namespace engine {
	GlobalConfig::GlobalConfig() {
		using json = nlohmann::json;
		std::ifstream file("Assets/config.json");
		json j;
		file >> j;

		m_scenePath = j["scenePath"];
	}

	GlobalConfig::~GlobalConfig() {

	}

	GlobalConfig* GlobalConfig::getInstance() {
		static GlobalConfig config;
		return &config;
	}
}
