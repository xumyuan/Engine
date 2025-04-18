#include "pch.h"
#include "Scene3D.h"

#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/common/Cube.h"
#include "graphics/mesh/common/Sphere.h"
#include "graphics/mesh/common/Quad.h"

#include "thread/thread_pool.h"

#include "utils/json/typeProcess.h"
#include "utils/json/json_type.h"
#include "utils/global_config/GlobalConfig.h"

#define NEW_LOAD 1

namespace engine {

	Scene3D::Scene3D(Window* window)
		:m_SceneCamera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f),
		m_ModelRenderer(getCamera()),
		m_Terrain(glm::vec3(-220.0f, 0.0f, 0.0f)),
		m_ProbeManager(m_SceneProbeBlendSetting)
	{
		m_GLCache = GLCache::getInstance();

		m_config = GlobalConfig::getInstance();

		init();
	}

	Scene3D::~Scene3D() {

	}

	void Scene3D::init() {
		using json = nlohmann::json;

		m_GLCache->setMultisample(true);
#ifdef NEW_LOAD
		try
		{
			std::ifstream file(m_config->getScenePath());
			json j;
			file >> j;

			SceneInfo sceneInfo = j.get<SceneInfo>();

			auto& modelList = sceneInfo.modelInfoList;

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
			// progress skybox info
			{
				auto& skyboxInfo = sceneInfo.skyboxInfo;
				auto& skyboxFilePaths = skyboxInfo.skyboxFilePaths;
				if (skyboxFilePaths.size() != 6) {
					spdlog::error("Skybox file paths are not valid");
				}
				else {
					m_Skybox = new Skybox(skyboxFilePaths);
					m_ProbeManager.init(m_Skybox);
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
#endif

#ifndef NEW_LOAD
		Model* pbrGun = new engine::Model("res/3D_Models/Cerberus_Gun/Cerberus_LP.FBX");
		auto& gunMat = pbrGun->getMeshes()[0].getMaterial();
		gunMat.setNormalMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_N.tga"));
		gunMat.setAmbientOcclusionMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_AO.tga"));
		gunMat.setMetallicMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_M.tga"));
		gunMat.setRoughnessMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_R.tga"));
		gunMat.SetAlbedoColour(glm::vec4(1.0f));


		m_RenderableModels.push_back(new RenderableModel(glm::vec3(120.0f, 130.0f, 100.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f), pbrGun, nullptr, false));

		// Skybox
		std::vector<std::string> skyboxFilePaths;
		skyboxFilePaths.push_back("res/skybox/right.png");
		skyboxFilePaths.push_back("res/skybox/left.png");
		skyboxFilePaths.push_back("res/skybox/top.png");
		skyboxFilePaths.push_back("res/skybox/bottom.png");
		skyboxFilePaths.push_back("res/skybox/back.png");
		skyboxFilePaths.push_back("res/skybox/front.png");
		m_Skybox = new Skybox(skyboxFilePaths);
		m_ProbeManager.init(m_Skybox);
#endif // !NEW_LOAD
		// Temp testing code
		/*int nrRows = 1;
		int nrColumns = 1;
		float spacing = 2.5;
		for (int row = 0; row < nrRows; row++) {
			for (int col = 0; col < nrColumns; col++) {
				Model* sphere = new engine::Model("res/3D_Models/Sphere/globe-sphere.obj");
				Material& mat = sphere->getMeshes()[0].getMaterial();
				mat.setAlbedoMap(TextureLoader::getDefaultAO());
				mat.setNormalMap(TextureLoader::getDefaultNormal());
				mat.setAmbientOcclusionMap(TextureLoader::getDefaultAO());
				mat.setMetallicMap(TextureLoader::getFullMetallic());
				mat.setRoughnessMap(TextureLoader::getNoRoughness());

				m_RenderableModels.push_back(
					new RenderableModel(
						glm::vec3((float)(col - (nrColumns / 2)) * spacing + 60,
							(float)(row - (nrRows / 2)) * spacing + 90, 130.0f),
						glm::vec3(20.0f, 20.0f, 20.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, sphere, nullptr, false, false));
			}
		}*/
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
