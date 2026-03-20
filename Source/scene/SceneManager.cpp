#include "pch.h"
#include "SceneManager.h"

#include "scene/Scene3D.h"
#include "scene/SceneLoader.h"

namespace engine {

	Scene3D* SceneManager::createScene(const std::string& name, Window* window) {
		if (hasScene(name)) {
			spdlog::warn("Scene '{}' already exists, returning existing scene", name);
			return m_Scenes[name].get();
		}

		auto scene = std::make_unique<Scene3D>(window);
		Scene3D* ptr = scene.get();
		m_Scenes[name] = std::move(scene);

		// 如果没有活跃场景，自动设为活跃
		if (!m_ActiveScene) {
			m_ActiveScene = ptr;
		}

		spdlog::info("Scene '{}' created", name);
		return ptr;
	}

	Scene3D* SceneManager::loadScene(const std::string& name, const std::string& filePath, Window* window) {
		if (hasScene(name)) {
			spdlog::warn("Scene '{}' already exists, destroying old scene first", name);
			destroyScene(name);
		}

		auto scene = std::make_unique<Scene3D>(window, filePath);
		Scene3D* ptr = scene.get();
		m_Scenes[name] = std::move(scene);

		// 如果没有活跃场景，自动设为活跃
		if (!m_ActiveScene) {
			m_ActiveScene = ptr;
		}

		spdlog::info("Scene '{}' loaded from '{}'", name, filePath);
		return ptr;
	}

	void SceneManager::destroyScene(const std::string& name) {
		auto it = m_Scenes.find(name);
		if (it == m_Scenes.end()) {
			spdlog::warn("Scene '{}' not found, cannot destroy", name);
			return;
		}

		// 如果销毁的是活跃场景，清空活跃指针
		if (m_ActiveScene == it->second.get()) {
			m_ActiveScene = nullptr;
		}

		m_Scenes.erase(it);
		spdlog::info("Scene '{}' destroyed", name);
	}

	void SceneManager::setActiveScene(const std::string& name) {
		auto it = m_Scenes.find(name);
		if (it == m_Scenes.end()) {
			spdlog::error("Scene '{}' not found, cannot set as active", name);
			return;
		}
		m_ActiveScene = it->second.get();
		spdlog::info("Active scene set to '{}'", name);
	}

	Scene3D* SceneManager::getScene(const std::string& name) const {
		auto it = m_Scenes.find(name);
		if (it != m_Scenes.end()) {
			return it->second.get();
		}
		return nullptr;
	}

	bool SceneManager::hasScene(const std::string& name) const {
		return m_Scenes.find(name) != m_Scenes.end();
	}

}
