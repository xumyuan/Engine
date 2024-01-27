#include "Scene3D.h"

#include <iterator>
#include <iostream>
#include <glm/glm.hpp>

namespace engine {
	Scene3D::Scene3D(graphics::Camera* camera, graphics::Window* window)
		: m_TerrainShader("src/shaders/basic.vert", "src/shaders/terrain.frag"), m_ModelShader("src/shaders/basic.vert", "src/shaders/model.frag"), m_Camera(camera),
		m_OutlineShader("src/shaders/basic.vert", "src/shaders/basic.frag"),
		m_DynamicLightManager()
	{
		m_Renderer = new graphics::Renderer(camera);
		m_GLCache = graphics::GLCache::getInstance();
		glm::vec3 worldpos = glm::vec3(0.0f, -20.0f, 0.0f);
		m_Terrain = new terrain::Terrain(worldpos);

		init();
	}

	Scene3D::~Scene3D() {

	}

	void Scene3D::init() {
		glEnable(GL_MULTISAMPLE);

		//开启深度测试和模板测试
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		//开启背面剔除
		glEnable(GL_CULL_FACE);


		std::vector<graphics::Mesh> meshes;
		meshes.push_back(*m_meshFactory.CreateQuad("res/textures/window.png", false));

		Add(new graphics::Renderable3D(
			glm::vec3(90.0f, -10.0f, 90.0f),
			glm::vec3(3.0f, 3.0f, 3.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			0,
			new engine::graphics::Model("res/3D_Models/nanosuit_model/nanosuit.obj"),
			nullptr, false, false));

		Add(new graphics::Renderable3D(
			glm::vec3(60.0f, 20.0f, 60.0f),
			glm::vec3(0.2f, 0.2f, 0.2f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			0,
			new engine::graphics::Model("res/3D_Models/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX"), nullptr, false));

		Add(new graphics::Renderable3D(
			glm::vec3(40, 20, 40),
			glm::vec3(15, 15, 15),
			glm::vec3(1.0, 0.0, 0.0),
			glm::radians(90.0f),
			new graphics::Model(meshes), nullptr, false, true));

		Add(new graphics::Renderable3D(
			glm::vec3(80, 20, 80),
			glm::vec3(15, 15, 15),
			glm::vec3(1.0, 0.0, 0.0),
			glm::radians(90.0f),
			new graphics::Model(meshes), nullptr, false, true));

		Add(new graphics::Renderable3D(
			glm::vec3(120, 20, 120),
			glm::vec3(15, 15, 15),
			glm::vec3(1.0, 0.0, 0.0),
			glm::radians(90.0f),
			new graphics::Model(meshes), nullptr, false, true));

		// 地形shader设置
		m_GLCache->switchShader(m_TerrainShader.getShaderID());
		m_TerrainShader.setUniform1f("material.shininess", 128.0f);
		m_DynamicLightManager.setupLightingUniforms(m_TerrainShader);

		// 模型shader
		m_GLCache->switchShader(m_ModelShader.getShaderID());
		m_ModelShader.setUniform1f("material.shininess", 128.0f);

		// Skybox
		std::vector<const char*> skyboxFilePaths;
		skyboxFilePaths.push_back("res/skybox/right.png");
		skyboxFilePaths.push_back("res/skybox/left.png");
		skyboxFilePaths.push_back("res/skybox/top.png");
		skyboxFilePaths.push_back("res/skybox/bottom.png");
		skyboxFilePaths.push_back("res/skybox/back.png");
		skyboxFilePaths.push_back("res/skybox/front.png");
		m_Skybox = new graphics::Skybox(skyboxFilePaths, m_Camera);
	}

	void Scene3D::onUpdate(float deltaTime) {
		//m_Renderables[0]->setRadianRotation(m_Renderables[0]->getRadianRotation() + deltaTime);
	}

	// 场景渲染
	void Scene3D::onRender() {
		//setup
		// 投影矩阵
		glm::mat4 projectionMat = glm::perspective(glm::radians(m_Camera->getFOV()),
			(float)graphics::Window::getWidth() / (float)graphics::Window::getHeight(), NEAR_PLANE, FAR_PLANE);

		m_DynamicLightManager.setSpotLightDirection(m_Camera->getFront());
		m_DynamicLightManager.setSpotLightPosition(m_Camera->getPosition());

		// 外轮廓
		m_GLCache->switchShader(m_OutlineShader.getShaderID());
		m_OutlineShader.setUniformMat4("view", m_Camera->getViewMatrix());
		m_OutlineShader.setUniformMat4("projection", projectionMat);


		// 模型渲染
		m_GLCache->switchShader(m_ModelShader.getShaderID());
		m_DynamicLightManager.setupLightingUniforms(m_ModelShader);
		m_ModelShader.setUniform3f("viewPos", m_Camera->getPosition());
		m_ModelShader.setUniformMat4("view", m_Camera->getViewMatrix());
		m_ModelShader.setUniformMat4("projection", projectionMat);

		std::vector<graphics::Renderable3D*>::iterator iter = m_Renderables.begin();
		while (iter != m_Renderables.end()) {
			graphics::Renderable3D* curr = *iter;
			if (curr->getTransparent()) {
				m_Renderer->submitTransparent(curr);
			}
			else {
				m_Renderer->submitOpaque(curr);
			}
			iter++;
		}

		m_Renderer->flushOpaque(m_ModelShader, m_OutlineShader);


		// 地形
		glStencilMask(0x00); // Don't update the stencil buffer
		m_GLCache->switchShader(m_TerrainShader.getShaderID());
		m_DynamicLightManager.setupLightingUniforms(m_TerrainShader);
		m_TerrainShader.setUniform3f("viewPos", m_Camera->getPosition());

		glm::mat4 modelMatrix(1);
		modelMatrix = glm::translate(modelMatrix, m_Terrain->getPosition());
		m_TerrainShader.setUniformMat4("model", modelMatrix);
		m_TerrainShader.setUniformMat4("view", m_Camera->getViewMatrix());
		m_TerrainShader.setUniformMat4("projection", projectionMat);
		m_Terrain->Draw(m_TerrainShader);

		m_Skybox->Draw();

		//透明物体渲染
		m_GLCache->switchShader(m_ModelShader.getShaderID());
		m_Renderer->flushTransparent(m_ModelShader, m_OutlineShader);
	}

	void Scene3D::Add(graphics::Renderable3D* renderable) {
		m_Renderables.push_back(renderable);
	}
}