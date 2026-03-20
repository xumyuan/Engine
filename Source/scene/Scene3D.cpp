#include "pch.h"
#include "Scene3D.h"
#include "SceneLoader.h"

#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/common/Cube.h"
#include "graphics/mesh/common/Sphere.h"
#include "graphics/mesh/common/Quad.h"
#include "physics/fluid/FluidSim.h"

#include "utils/global_config/GlobalConfig.h"

namespace engine {

	Scene3D::Scene3D(Window* window)
		:m_SceneCamera(glm::vec3(105.716f, 136.20f, 98.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f),
		m_ModelRenderer(getCamera()),
		m_Terrain(glm::vec3(-220.0f, 0.0f, 0.0f)),
		m_ProbeManager(m_SceneProbeBlendSetting)
	{
		m_config = GlobalConfig::getInstance();

		// 初始化 RHI 管线状态
		if (auto* device = getRHIDevice()) {
			rhi::PipelineState initPipeline;
			initPipeline.multisample = true;
			device->bindPipeline(initPipeline);
		}

		// 通过 SceneLoader 加载场景数据（从 JSON 解析模型、天空盒、灯光等）
		BEGIN_EVENT("Scene Init");
		SceneLoader::loadFromFile(m_config->getScenePath(), *this);
		END_EVENT();
	}

	Scene3D::Scene3D(Window* window, const std::string& scenePath)
		:m_SceneCamera(glm::vec3(105.716f, 136.20f, 98.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f),
		m_ModelRenderer(getCamera()),
		m_Terrain(glm::vec3(-220.0f, 0.0f, 0.0f)),
		m_ProbeManager(m_SceneProbeBlendSetting)
	{
		m_config = GlobalConfig::getInstance();

		// 初始化 RHI 管线状态
		if (auto* device = getRHIDevice()) {
			rhi::PipelineState initPipeline;
			initPipeline.multisample = true;
			device->bindPipeline(initPipeline);
		}

		// 通过 SceneLoader 从指定路径加载场景数据
		BEGIN_EVENT("Scene Init");
		SceneLoader::loadFromFile(scenePath, *this);
		END_EVENT();
	}

	Scene3D::~Scene3D() {
		// 释放所有渲染模型（RenderableModel 析构会释放内部的 Model 和子节点）
		for (auto* model : m_RenderableModels) {
			delete model;
		}
		m_RenderableModels.clear();

		// 释放天空盒
		if (m_Skybox) {
			delete m_Skybox;
			m_Skybox = nullptr;
		}

		// 释放流体模拟
		if (m_fluid) {
			delete m_fluid;
			m_fluid = nullptr;
		}
	}

	void Scene3D::addRenderableModel(RenderableModel* model) {
		if (model) {
			m_RenderableModels.push_back(model);
		}
	}

	void Scene3D::setSkybox(Skybox* skybox) {
		// 释放旧的天空盒
		if (m_Skybox) {
			delete m_Skybox;
		}
		m_Skybox = skybox;
	}

	void Scene3D::onUpdate(float deltaTime) {
		// Camera Update
		m_SceneCamera.processInput(deltaTime);

		m_DynamicLightManager.setSpotLightDirection(m_SceneCamera.getFront());
		m_DynamicLightManager.setSpotLightPosition(m_SceneCamera.getPosition());
	}

	// 场景渲染
	void Scene3D::onRender() {
	}

	RenderScene Scene3D::extractRenderScene() {
		RenderScene rs;
		rs.camera = &m_SceneCamera;
		rs.modelRenderer = &m_ModelRenderer;
		rs.skybox = m_Skybox;
		rs.terrain = &m_Terrain;
		rs.lightManager = &m_DynamicLightManager;
		rs.probeManager = &m_ProbeManager;
		rs.fluid = m_fluid;
		rs.renderableModels = &m_RenderableModels;
		return rs;
	}

	void Scene3D::addModelsToRenderer() {
		auto iter = m_RenderableModels.begin();
		while (iter != m_RenderableModels.end()) {
			RenderableModel* curr = *iter;
			if (curr->getTransparent()) {
				m_ModelRenderer.submitTransparent(curr);
			}
			else {
				m_ModelRenderer.submitOpaque(curr);
			}

			iter++;
		}
	}
}
