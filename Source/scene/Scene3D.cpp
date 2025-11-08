#include "pch.h"
#include "Scene3D.h"

#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/common/Cube.h"
#include "graphics/mesh/common/Sphere.h"
#include "graphics/mesh/common/Quad.h"
#include "physics/fluid/FluidSim.h"

#include "thread/thread_pool.h"

#include "utils/json/typeProcess.h"
#include "utils/json/json_type.h"
#include "utils/json/JsonUtils.h"
#include "utils/global_config/GlobalConfig.h"

namespace engine {

	Scene3D::Scene3D(Window* window)
		:m_SceneCamera(glm::vec3(105.716f, 136.20f, 98.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f),
		m_ModelRenderer(getCamera()),
		m_Terrain(glm::vec3(-220.0f, 0.0f, 0.0f)),
		m_ProbeManager(m_SceneProbeBlendSetting)
	{
		m_GLCache = GLCache::getInstance();

		m_config = GlobalConfig::getInstance();

		/*m_fluid = new FluidSim(8'000, { { 120.0f,120.0f,120.0f }, { 140.0f,200.0f,180.0f} });
		std::thread simThread(&FluidSim::startSim, m_fluid);
		simThread.detach();*/
		BEGIN_EVENT("Scene Init");
		init();
		END_EVENT();
	}

	Scene3D::~Scene3D() {

	}

	void Scene3D::init() {
		using json = nlohmann::json;

		m_GLCache->setMultisample(true);

		// parse scene json
		try
		{
			// 使用增强的JSON解析工具
			json j = JsonUtils::loadJsonFromFile(m_config->getScenePath());
			
			// 由于所有字段都是可选的，我们只需要验证JSON是否有效
			SceneInfo sceneInfo = j.get<SceneInfo>();

			// 处理模型列表
			auto& modelList = sceneInfo.modelInfoList;
			if (!modelList.empty()) {
				for (auto& model : modelList) {
				std::string& modelPath = model.modelPath;
				glm::vec3& position = model.position;
				glm::vec3& scale = model.scale;
				glm::vec3& rotationAxis = model.rotationAxis;
				float radianRotation = model.radianRotation;
				bool isStatic = model.isStatic;
				bool isTransparent = model.isTransparent;
				Model* modelPtr = new engine::Model(modelPath.c_str());

				if (!model.customMatTexList.empty())
				{
					auto& modelMat = modelPtr->getMeshes()[0].getMaterial();
					modelMat.processMaterial(model);
				}

					m_RenderableModels.push_back(new RenderableModel(position, scale, rotationAxis, glm::radians(radianRotation), modelPtr, nullptr, isStatic, isTransparent));
				}
			}
			// progress skybox info
			{
				auto& skyboxInfo = sceneInfo.skyboxInfo;
				auto& skyboxFilePaths = skyboxInfo.skyboxFilePaths;
				if (!skyboxFilePaths.empty()) {
					if (skyboxFilePaths.size() != 6) {
						spdlog::error("Skybox file paths are not valid");
					}
					else {
						m_Skybox = new Skybox(skyboxFilePaths);
						m_ProbeManager.init(m_Skybox);
					}
				}
			}

			// progress lights
			{
				auto& lightsInfo = sceneInfo.lightsInfo;
				
				// 设置方向光
				if (lightsInfo.directionalLight.isActive) {
					m_DynamicLightManager.setDirectionalLight(
						lightsInfo.directionalLight.direction,
						lightsInfo.directionalLight.lightColor
					);
				}
				
				// 设置聚光灯
				if (lightsInfo.spotLight.isActive) {
					m_DynamicLightManager.setSpotLight(
						lightsInfo.spotLight.position,
						lightsInfo.spotLight.direction,
						lightsInfo.spotLight.lightColor,
						lightsInfo.spotLight.cutOff,
						lightsInfo.spotLight.outerCutOff
					);
				}
				
				// 设置点光源并添加光球模型
				if(0)
				for (auto& pointLight : lightsInfo.pointLightList) {
					if (pointLight.isActive) {
						m_DynamicLightManager.addPointLight(
							pointLight.position,
							pointLight.lightColor
						);
						
						// 创建光球模型
						Sphere* lightSphere = new Sphere(10, 10);
						Model* lightSphereModel = new Model(*lightSphere);
						
						// 设置材质为光源颜色
						auto& material = lightSphereModel->getMeshes()[0].getMaterial();
						material.SetAlbedoColour(glm::vec4(pointLight.lightColor,1.0f));
						material.SetEmissionColour(glm::vec4(pointLight.lightColor,1.0f));
						
						// 添加到渲染列表
						m_RenderableModels.push_back(new RenderableModel(
							pointLight.position, 
							glm::vec3(5.0f, 5.0f, 5.0f), // 光球大小
							glm::vec3(0.0f, 1.0f, 0.0f), 
							0.0f, 
							lightSphereModel, 
							nullptr, 
							true, 
							false
						));
					}
				}
			}

		}
		catch (const json::exception& e)
		{
			spdlog::error("JSON parsing error: {}", e.what());
		}
		catch (const std::exception& e)
		{
			spdlog::error("Error: {}", e.what());
		}

		thread_pool.wait();
		TextureLoader::processMainThreadTasks();
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
