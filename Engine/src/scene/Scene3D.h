#pragma once

#include "graphics/Skybox.h"
#include "graphics/Window.h"
#include "graphics/camera/FPSCamera.h"
#include "graphics/dynamic lights/DynamicLightManager.h"
#include "graphics/ibl/EnvironmentProbeManager.h"
#include "graphics/renderer/GLCache.h"
#include "graphics/renderer/ModelRenderer.h"
#include "scene/RenderableModel.h"
#include "terrain/Terrain.h"
#include "utils/loaders/TextureLoader.h"

namespace engine {

	class Scene3D {

	public:
		Scene3D(Window* window);
		~Scene3D();


		void onUpdate(float deltaTime);
		void onRender();

		void addModelsToRenderer();

		inline ModelRenderer* getModelRenderer() { return &m_ModelRenderer; }
		inline Terrain* getTerrain() { return &m_Terrain; }
		inline DynamicLightManager* getDynamicLightManager() { return &m_DynamicLightManager; }
		inline EnvironmentProbeManager* getProbeManager() { return &m_ProbeManager; }
		inline FPSCamera* getCamera() { return &m_SceneCamera; }
		inline Skybox* getSkybox() { return m_Skybox; }
	private:
		void init();

	private:
		// Global Data
		GLCache* m_GLCache;

		// Scene Specific Data
		FPSCamera m_SceneCamera;
		Skybox* m_Skybox;
		ModelRenderer m_ModelRenderer;
		Terrain m_Terrain;
		DynamicLightManager m_DynamicLightManager;
		EnvironmentProbeManager m_ProbeManager;
		std::vector<RenderableModel*> m_RenderableModels;

		// Scene parameters
		EnvironmentProbeBlendSetting m_SceneProbeBlendSetting = PROBES_SIMPLE;

	};

}