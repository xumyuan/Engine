#pragma once
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include "utils/Singleton.h"

namespace engine {
	class GlobalConfig : public Singleton<GlobalConfig>
	{
		friend class Singleton<GlobalConfig>;
	public:
		~GlobalConfig();

		/// @brief 从指定路径加载配置文件（构造与加载分离）
		/// @param path 配置文件路径，默认为 "Assets/config.json"
		/// @return 加载成功返回 true，失败返回 false
		bool loadFromFile(const std::string& path = "Assets/config.json");

		/// @brief 通用类型安全取值接口，支持默认值
		/// @tparam T 值类型
		/// @param key JSON 键名
		/// @param defaultValue 键不存在时返回的默认值
		/// @return 配置值或默认值
		template<typename T>
		T get(const std::string& key, const T& defaultValue = T{}) const {
			if (m_data.contains(key)) {
				try {
					return m_data[key].get<T>();
				}
				catch (const nlohmann::json::type_error& e) {
					std::cerr << "[GlobalConfig] 类型转换错误, key=" << key << ": " << e.what() << std::endl;
					return defaultValue;
				}
			}
			return defaultValue;
		}

		/// @brief 配置是否已成功加载
		bool isLoaded() const { return m_loaded; }

		// ── 便捷接口（常用配置的语义化 getter） ──
		std::string getScenePath() const { return get<std::string>("scenePath"); }

	private:
		GlobalConfig();

		nlohmann::json m_data;  ///< 存储全部配置数据
		bool m_loaded = false;  ///< 标记配置是否已成功加载
	};
}


