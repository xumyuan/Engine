#include "pch.h"
#include "SceneLoader.h"

#include "scene/Scene3D.h"
#include "scene/RenderableModel.h"
#include "graphics/mesh/Model.h"
#include "graphics/mesh/common/Sphere.h"
#include "graphics/Skybox.h"

#include "utils/json/TypeProcess.h"
#include "utils/json/JsonType.h"
#include "utils/json/JsonUtils.h"

#include "thread/ThreadPool.h"
#include "utils/loaders/TextureLoader.h"

namespace engine {

	void SceneLoader::loadFromFile(const std::string& filePath, Scene3D& scene) {
		using json = nlohmann::json;

		try
		{
			// 使用增强的JSON解析工具
			json j = JsonUtils::loadJsonFromFile(filePath);

			// 由于所有字段都是可选的，我们只需要验证JSON是否有效
			SceneInfo sceneInfo = j.get<SceneInfo>();

			// 按职责分别加载各类数据
			loadModels(scene, sceneInfo);
			loadSkybox(scene, sceneInfo);
			loadLights(scene, sceneInfo);
		}
		catch (const json::exception& e)
		{
			spdlog::error("JSON parsing error: {}", e.what());
		}
		catch (const std::exception& e)
		{
			spdlog::error("Error: {}", e.what());
		}

		// 等待线程池中的异步任务（如纹理加载）完成
		thread_pool.wait();
		TextureLoader::processMainThreadTasks();
	}

	void SceneLoader::loadModels(Scene3D& scene, const SceneInfo& sceneInfo) {
		auto& modelList = sceneInfo.modelInfoList;
		if (modelList.empty()) return;

		for (auto& modelInfo : modelList) {
			std::string modelPath = modelInfo.modelPath;
			glm::vec3 position = modelInfo.position;
			glm::vec3 scale = modelInfo.scale;
			glm::vec3 rotationAxis = modelInfo.rotationAxis;
			float radianRotation = modelInfo.radianRotation;
			bool isStatic = modelInfo.isStatic;
			bool isTransparent = modelInfo.isTransparent;

			Model* modelPtr = new engine::Model(modelPath.c_str());

			if (!modelInfo.customMatTexList.empty())
			{
				auto& modelMat = modelPtr->getMeshes()[0].getMaterial();
				modelMat.processMaterial(modelInfo);
			}

			scene.addRenderableModel(new RenderableModel(
				position, scale, rotationAxis,
				glm::radians(radianRotation),
				modelPtr, nullptr, isStatic, isTransparent
			));
		}
	}

	void SceneLoader::loadSkybox(Scene3D& scene, const SceneInfo& sceneInfo) {
		auto& skyboxInfo = sceneInfo.skyboxInfo;
		auto& skyboxFilePaths = skyboxInfo.skyboxFilePaths;

		if (skyboxFilePaths.empty()) return;

		if (skyboxFilePaths.size() != 6) {
			spdlog::error("Skybox file paths are not valid");
			return;
		}

		scene.setSkybox(new Skybox(skyboxFilePaths));
		scene.getProbeManager()->init(scene.getSkybox());
	}

	void SceneLoader::loadLights(Scene3D& scene, const SceneInfo& sceneInfo) {
		auto& lightsInfo = sceneInfo.lightsInfo;
		DynamicLightManager* lightManager = scene.getDynamicLightManager();

		// 设置方向光
		if (lightsInfo.directionalLight.isActive) {
			lightManager->setDirectionalLight(
				lightsInfo.directionalLight.direction,
				lightsInfo.directionalLight.lightColor
			);
		}

		// 设置聚光灯
		if (lightsInfo.spotLight.isActive) {
			lightManager->setSpotLight(
				lightsInfo.spotLight.position,
				lightsInfo.spotLight.direction,
				lightsInfo.spotLight.lightColor,
				lightsInfo.spotLight.cutOff,
				lightsInfo.spotLight.outerCutOff
			);
		}

		// 设置点光源并添加光球模型
		if (0)
		for (auto& pointLight : lightsInfo.pointLightList) {
			if (pointLight.isActive) {
				lightManager->addPointLight(
					pointLight.position,
					pointLight.lightColor
				);

				// 创建光球模型
				Sphere* lightSphere = new Sphere(10, 10);
				Model* lightSphereModel = new Model(std::move(*lightSphere));

				// 设置材质为光源颜色
				auto& material = lightSphereModel->getMeshes()[0].getMaterial();
				material.SetAlbedoColour(glm::vec4(pointLight.lightColor, 1.0f));
				material.SetEmissionColour(glm::vec4(pointLight.lightColor, 1.0f));

				// 添加到渲染列表
				scene.addRenderableModel(new RenderableModel(
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
