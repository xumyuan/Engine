#pragma once

#include "graphics/Skybox.h"
#include "graphics/Window.h"
#include "graphics/camera/FPSCamera.h"
#include "graphics/dynamic_lights/DynamicLightManager.h"
#include "graphics/ibl/ProbeManager.h"
#include "graphics/renderer/ModelRenderer.h"
#include "scene/RenderableModel.h"
#include "terrain/Terrain.h"
#include "utils/loaders/TextureLoader.h"
#include "scene/RenderScene.h"

namespace engine {

	class GlobalConfig;
	class FluidSim;
	class SceneLoader;

	class Scene3D {

		friend class SceneLoader;

	public:
		/// @brief 从 GlobalConfig 获取场景路径自动加载（兼容旧代码）
		Scene3D(Window* window);

		/// @brief 从指定路径加载场景（供 SceneManager 使用）
		Scene3D(Window* window, const std::string& scenePath);

		~Scene3D();


		void onUpdate(float deltaTime);
		void onRender();

		void addModelsToRenderer();

		/// @brief 提取渲染场景数据快照，供 MasterRenderer 和 RenderPass 使用
		RenderScene extractRenderScene();

		// 数据注入接口（供 SceneLoader 等外部模块使用）
		void addRenderableModel(RenderableModel* model);
		void setSkybox(Skybox* skybox);

		inline ModelRenderer* getModelRenderer() { return &m_ModelRenderer; }
		inline Terrain* getTerrain() { return &m_Terrain; }
		inline FluidSim* getFluid() { return m_fluid; }
		inline DynamicLightManager* getDynamicLightManager() { return &m_DynamicLightManager; }
		inline ProbeManager* getProbeManager() { return &m_ProbeManager; }
		inline FPSCamera* getCamera() { return &m_SceneCamera; }
		inline Skybox* getSkybox() { return m_Skybox; }

	private:
		// Global Config
		GlobalConfig* m_config;

		// Scene parameters
		ProbeBlendSetting m_SceneProbeBlendSetting = PROBES_SIMPLE;

		// Scene Specific Data
		FPSCamera m_SceneCamera;
		Skybox* m_Skybox = nullptr;
		ModelRenderer m_ModelRenderer;
		Terrain m_Terrain;
		FluidSim* m_fluid = nullptr;
		DynamicLightManager m_DynamicLightManager;
		ProbeManager m_ProbeManager;
		std::vector<RenderableModel*> m_RenderableModels;

	};

}
