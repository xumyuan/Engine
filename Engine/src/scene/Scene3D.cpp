#include "pch.h"
#include "Scene3D.h"

#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/common/Cube.h"
#include "graphics/mesh/common/Sphere.h"
#include "graphics/mesh/common/Quad.h"

#include "thread/thread_pool.h"

namespace engine {

	struct SceneJSON
	{
		struct ModelJSON
		{
			std::string modelPath;
			glm::vec3 position;
			glm::vec3 scale;
			glm::vec3 rotationAxis;
			float radianRotation;
			bool isStatic;
			bool isTransparent;
			std::unordered_map<std::string, std::string> customMatTexList;
		};
	};


	Scene3D::Scene3D(Window* window)
		:m_SceneCamera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f),
		m_ModelRenderer(getCamera()),
		m_Terrain(glm::vec3(-220.0f, 0.0f, 0.0f)),
		m_ProbeManager(m_SceneProbeBlendSetting)
	{
		m_GLCache = GLCache::getInstance();

		init();
	}

	Scene3D::~Scene3D() {

	}

	void Scene3D::init() {
		m_GLCache->setMultisample(true);
		// 纳米装模型
		/*m_RenderableModels.push_back(new Renderable3D(
			glm::vec3(90.0f, 60.0f, 90.0f),
			glm::vec3(3.0f, 3.0f, 3.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			0,
			new engine::Model("res/3D_Models/nanosuit_model/nanosuit.obj"),
			nullptr, false));*/

			//pbr 临时代码
		//Model* McCree = new engine::Model("res/3D_Models/Overwatch/McCree/McCree.FBX");

		Model* pbrGun = new engine::Model("res/3D_Models/Cerberus_Gun/Cerberus_LP.FBX");
		auto& gunMat = pbrGun->getMeshes()[0].getMaterial();
		gunMat.setNormalMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_N.tga"));
		gunMat.setAmbientOcclusionMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_AO.tga"));
		gunMat.setMetallicMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_M.tga"));
		gunMat.setRoughnessMap(TextureLoader::load2DTexture("res/3D_Models/Cerberus_Gun/Textures/Cerberus_R.tga"));
		gunMat.SetAlbedoColour(glm::vec4(1.0f));


		m_RenderableModels.push_back(new RenderableModel(glm::vec3(120.0f, 130.0f, 100.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f), pbrGun, nullptr, false));

		/*m_RenderableModels.push_back(new RenderableModel(glm::vec3(100.0f, 60.0f, 120.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f), McCree, nullptr, false));*/
		/*pbrGun->getMeshes()[0].getMaterial().setAlbedoMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_A.tga"), true));
		pbrGun->getMeshes()[0].getMaterial().setNormalMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_N.tga"), false));
		pbrGun->getMeshes()[0].getMaterial().setMetallicMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_M.tga"), false));
		pbrGun->getMeshes()[0].getMaterial().setRoughnessMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_R.tga"), false));
		pbrGun->getMeshes()[0].getMaterial().setAmbientOcclusionMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_AO.tga"), false));*/

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