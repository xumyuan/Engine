#include "pch.h"
#include "SceneLoader.h"

#include "scene/Scene3D.h"
#include "scene/RenderableModel.h"
#include "scene/SceneNode.h"
#include "scene/components/MeshComponent.h"
#include "scene/components/SkyboxComponent.h"
#include "scene/components/LightComponent.h"
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

			// ── [向后兼容] RenderableModel 仍然持有 Model 所有权（用于渲染管线） ──
			RenderableModel* renderableModel = new RenderableModel(
				position, scale, rotationAxis,
				glm::radians(radianRotation),
				modelPtr, nullptr, isStatic, isTransparent
			);
			scene.addRenderableModel(renderableModel);

			// ── 创建 SceneNode + MeshComponent（阶段三） ──
			// MeshComponent 不持有 Model 所有权（model 参数传 nullptr），
			// 仅记录节点的 Transform 和渲染属性，Model 生命周期仍由 RenderableModel 管理
			// 后续阶段将完全迁移到 MeshComponent 持有 Model 所有权
			auto* node = new SceneNode(modelPath);
			node->setPosition(position);
			node->setScale(scale);
			node->setOrientation(glm::radians(radianRotation), rotationAxis);
			node->addComponent(new MeshComponent(nullptr, isStatic, isTransparent));
			scene.addSceneNode(node);
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

		Skybox* skybox = new Skybox(skyboxFilePaths);

		// ── 创建 SceneNode + SkyboxComponent（阶段三） ──
		auto* skyboxNode = new SceneNode("Skybox");
		skyboxNode->addComponent(new SkyboxComponent(nullptr)); // SkyboxComponent 不持有所有权，由 Scene3D 管理
		scene.addSceneNode(skyboxNode);

		// ── [向后兼容] 旧接口 ──
		scene.setSkybox(skybox);
		scene.getProbeManager()->init(scene.getSkybox());
	}

	void SceneLoader::loadLights(Scene3D& scene, const SceneInfo& sceneInfo) {
		auto& lightsInfo = sceneInfo.lightsInfo;

		// 创建方向光 SceneNode + LightComponent
		if (lightsInfo.directionalLight.isActive) {
			auto* dirLightNode = new SceneNode("DirectionalLight");
			auto* lightComp = new LightComponent(LightType::Directional);
			lightComp->setDirection(lightsInfo.directionalLight.direction);
			lightComp->setLightColor(lightsInfo.directionalLight.lightColor);
			lightComp->setActive(true);
			dirLightNode->addComponent(lightComp);
			scene.addSceneNode(dirLightNode);
		}

		// 创建聚光灯 SceneNode + LightComponent
		if (lightsInfo.spotLight.isActive) {
			auto* spotLightNode = new SceneNode("SpotLight");
			spotLightNode->setPosition(lightsInfo.spotLight.position);
			auto* spotComp = new LightComponent(LightType::Spot);
			spotComp->setDirection(lightsInfo.spotLight.direction);
			spotComp->setLightColor(lightsInfo.spotLight.lightColor);
			spotComp->setCutOff(glm::cos(glm::radians(lightsInfo.spotLight.cutOff)));
			spotComp->setOuterCutOff(glm::cos(glm::radians(lightsInfo.spotLight.outerCutOff)));
			spotComp->setActive(true);
			spotLightNode->addComponent(spotComp);
			scene.addSceneNode(spotLightNode);
		}

		// 创建点光源 SceneNode + LightComponent + MeshComponent（光球模型）
		for (auto& pointLight : lightsInfo.pointLightList) {
			if (pointLight.isActive) {
				auto* pointLightNode = new SceneNode("PointLight");
				pointLightNode->setPosition(pointLight.position);
				pointLightNode->setScale(glm::vec3(5.0f));

				auto* pointComp = new LightComponent(LightType::Point);
				pointComp->setLightColor(pointLight.lightColor);
				pointComp->setActive(true);
				pointLightNode->addComponent(pointComp);

				// 创建光球模型
				Sphere* lightSphere = new Sphere(10, 10);
				Model* lightSphereModel = new Model(std::move(*lightSphere));

				// 设置材质为光源颜色
				auto& material = lightSphereModel->getMeshes()[0].getMaterial();
				material.SetAlbedoColour(glm::vec4(pointLight.lightColor, 1.0f));
				material.SetEmissionColour(glm::vec4(pointLight.lightColor, 1.0f));

				pointLightNode->addComponent(new MeshComponent(lightSphereModel, true, false));
				scene.addSceneNode(pointLightNode);

				// [向后兼容] 为旧渲染管线添加 RenderableModel
				scene.addRenderableModel(new RenderableModel(
					pointLight.position,
					glm::vec3(5.0f, 5.0f, 5.0f),
					glm::vec3(0.0f, 1.0f, 0.0f),
					0.0f,
					nullptr,
					nullptr,
					true,
					false
				));
			}
		}
	}

}
