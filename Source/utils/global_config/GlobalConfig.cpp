#include "pch.h"
#include "GlobalConfig.h"
#include <fstream>

namespace engine {
	GlobalConfig::GlobalConfig() {
		// 构造与加载分离，不再在构造函数中自动加载
	}

	GlobalConfig::~GlobalConfig() {

	}

	bool GlobalConfig::loadFromFile(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "[GlobalConfig] 无法打开配置文件: " << path << std::endl;
			return false;
		}

		try {
			file >> m_data;
			m_loaded = true;
		}
		catch (const nlohmann::json::parse_error& e) {
			std::cerr << "[GlobalConfig] JSON 解析错误: " << e.what() << std::endl;
			m_loaded = false;
			return false;
		}

		return true;
	}
}
