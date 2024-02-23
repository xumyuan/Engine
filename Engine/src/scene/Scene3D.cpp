#include "pch.h"
#include "Scene3D.h"

#include "graphics/mesh/Mesh.h"
#include "graphics/mesh/common/Cube.h"
#include "graphics/mesh/common/Sphere.h"
#include "graphics/mesh/common/Quad.h"

namespace engine {
	Scene3D::Scene3D(Window* window)
		:m_SceneCamera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f),
		m_TerrainShader("src/shaders/terrain.vert", "src/shaders/terrain.frag"),
		m_ModelShader("src/shaders/pbr_model.vert", "src/shaders/pbr_model.frag"),
		m_ShadowmapShader("src/shaders/shadowmap.vert", "src/shaders/shadowmap.frag"),
		m_DynamicLightManager(),
		m_ModelRenderer(getCamera()), m_Terrain(glm::vec3(0.0f, -20.0f, 0.0f))
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
		Model* pbrGun = new engine::Model("res/3D_Models/Cerberus_Gun/Cerberus_LP.FBX");
		m_RenderableModels.push_back(new RenderableModel(glm::vec3(120.0f, 75.0f, 120.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f), pbrGun, nullptr, false));
		pbrGun->getMeshes()[0].getMaterial().setAlbedoMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_A.tga"), true));
		pbrGun->getMeshes()[0].getMaterial().setNormalMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_N.tga"), false));
		pbrGun->getMeshes()[0].getMaterial().setMetallicMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_M.tga"), false));
		pbrGun->getMeshes()[0].getMaterial().setRoughnessMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_R.tga"), false));
		pbrGun->getMeshes()[0].getMaterial().setAmbientOcclusionMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Cerberus_Gun/Textures/Cerberus_AO.tga"), false));

		// Temp testing code
		/*int nrRows = 2;
		int nrColumns = 2;
		float spacing = 2.5;
		for (int row = 0; row < nrRows; row++) {
			for (int col = 0; col < nrColumns; col++) {
				Model* sphere = new engine::Model("res/3D_Models/Sphere/globe-sphere.obj");
				Material& mat = sphere->getMeshes()[0].getMaterial();
				mat.setAlbedoMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Sphere/rustediron2_basecolor.png"), true));
				mat.setNormalMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Sphere/rustediron2_normal.png"), false));
				mat.setAmbientOcclusionMap(TextureLoader::load2DTexture(std::string("res/textures/default/white.png"), false));
				mat.setMetallicMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Sphere/rustediron2_metallic.png"), false));
				mat.setRoughnessMap(TextureLoader::load2DTexture(std::string("res/3D_Models/Sphere/rustediron2_roughness.png"), false));
				m_RenderableModels.push_back(new Renderable3D(glm::vec3((float)(col - (nrColumns / 2)) * spacing,
					(float)(row - (nrRows / 2)) * spacing, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, sphere, nullptr, false));
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
		m_Skybox = new Skybox(skyboxFilePaths, getCamera());
	}

	void Scene3D::shadowmapPass() {
		glm::vec3 dirLightShadowmapLookAtPos = m_SceneCamera.getPosition() + (glm::normalize(glm::vec3(m_SceneCamera.getFront().x, 0.0f, m_SceneCamera.getFront().z)) * 50.0f);
		glm::vec3 dirLightShadowmapEyePos = dirLightShadowmapLookAtPos + (-m_DynamicLightManager.getDirectionalLightDirection() * 100.0f);

		m_GLCache->switchShader(m_ShadowmapShader.getShaderID());
		glm::mat4 directionalLightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, SHADOWMAP_NEAR_PLANE, SHADOWMAP_FAR_PLANE);
		glm::mat4 directionalLightView = glm::lookAt(dirLightShadowmapEyePos, dirLightShadowmapLookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 directionalLightViewProjMatrix = directionalLightProjection * directionalLightView;
		m_ShadowmapShader.setUniformMat4("lightSpaceViewProjectionMatrix", directionalLightViewProjMatrix);

		// Add objects to the renderer
		addModelsToRenderer();

		m_ModelRenderer.flushOpaque(m_ShadowmapShader, RenderPassType::ShadowmapPass);
		m_ModelRenderer.flushTransparent(m_ShadowmapShader, RenderPassType::ShadowmapPass);
		m_Terrain.Draw(m_ShadowmapShader, RenderPassType::ShadowmapPass);

		m_GLCache->switchShader(m_TerrainShader.getShaderID());
		m_TerrainShader.setUniformMat4("lightSpaceViewProjectionMatrix", directionalLightViewProjMatrix);

		m_GLCache->switchShader(m_ModelShader.getShaderID());
		m_ModelShader.setUniformMat4("lightSpaceViewProjectionMatrix", directionalLightViewProjMatrix);
	}

	float rotAmount = 0.0f;
	void Scene3D::onUpdate(float deltaTime) {
		// Camera Update
		m_SceneCamera.processInput(deltaTime);
	}

	// 场景渲染
	void Scene3D::onRender(unsigned int shadowmap) {
		//setup
		// 投影矩阵
		glm::mat4 projectionMat = m_SceneCamera.getProjectionMatrix();

		m_DynamicLightManager.setSpotLightDirection(m_SceneCamera.getFront());
		m_DynamicLightManager.setSpotLightPosition(m_SceneCamera.getPosition());

		// 模型渲染
		m_GLCache->switchShader(m_ModelShader.getShaderID());
		m_DynamicLightManager.setupLightingUniforms(m_ModelShader);
		m_ModelShader.setUniform3f("viewPos", m_SceneCamera.getPosition());
		m_ModelShader.setUniformMat4("view", m_SceneCamera.getViewMatrix());
		m_ModelShader.setUniformMat4("projection", projectionMat);

		// Shadow map code
		m_ModelShader.setUniform1i("shadowmap", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowmap);

		// IBL
		m_ModelShader.setUniform1i("irradianceMap", 1);
		m_Skybox->getSkyboxCubemap()->bind(1);

		// Add objects to the renderer
		addModelsToRenderer();

		// Opaque objects
		m_ModelRenderer.flushOpaque(m_ModelShader, RenderPassType::LightingPass);

		// 地形
		m_GLCache->switchShader(m_TerrainShader.getShaderID());

		m_TerrainShader.setUniform1i("shadowmap", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowmap);

		m_DynamicLightManager.setupLightingUniforms(m_TerrainShader);
		m_TerrainShader.setUniform3f("viewPos", m_SceneCamera.getPosition());

		glm::mat4 modelMatrix(1);
		modelMatrix = glm::translate(modelMatrix, m_Terrain.getPosition());
		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
		m_TerrainShader.setUniformMat3("normalMatrix", normalMatrix);
		m_TerrainShader.setUniformMat4("model", modelMatrix);
		m_TerrainShader.setUniformMat4("view", m_SceneCamera.getViewMatrix());
		m_TerrainShader.setUniformMat4("projection", projectionMat);
		m_Terrain.Draw(m_TerrainShader, RenderPassType::LightingPass);

		// 天空盒
		m_Skybox->Draw();

		//透明物体渲染
		m_GLCache->switchShader(m_ModelShader.getShaderID());
		m_ModelRenderer.flushTransparent(m_ModelShader, RenderPassType::LightingPass);

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