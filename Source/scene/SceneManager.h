#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace engine {

	class Scene3D;
	class Window;

	/// @brief 场景管理器 —— 管理场景的创建、销毁、切换，支持多场景
	class SceneManager {
	public:
		SceneManager() = default;
		~SceneManager() = default;

		// 禁止拷贝
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

		/// @brief 创建一个空场景并注册
		/// @param name 场景名称（唯一标识）
		/// @param window 窗口指针
		/// @return 创建的场景指针（生命周期由 SceneManager 管理）
		Scene3D* createScene(const std::string& name, Window* window);

		/// @brief 从 JSON 文件加载场景
		/// @param name 场景名称
		/// @param filePath JSON 文件路径
		/// @param window 窗口指针
		/// @return 加载的场景指针
		Scene3D* loadScene(const std::string& name, const std::string& filePath, Window* window);

		/// @brief 销毁指定场景
		void destroyScene(const std::string& name);

		/// @brief 设置当前活跃场景
		void setActiveScene(const std::string& name);

		/// @brief 获取当前活跃场景
		Scene3D* getActiveScene() const { return m_ActiveScene; }

		/// @brief 根据名称获取场景
		Scene3D* getScene(const std::string& name) const;

		/// @brief 场景是否存在
		bool hasScene(const std::string& name) const;

	private:
		std::unordered_map<std::string, std::unique_ptr<Scene3D>> m_Scenes;
		Scene3D* m_ActiveScene = nullptr;
	};

}
