#pragma once

#include "graphics\renderer\Renderable3D.h"
#include "graphics\camera\FPSCamera.h"
#include "graphics\renderer\Renderer.h"
#include "terrain\Terrain.h"
#include "graphics\Window.h"
#include "graphics\MeshFactory.h"

namespace engine {

	class Scene3D {
	private:
		graphics::Window* m_Window;
		graphics::FPSCamera* m_Camera;
		graphics::Renderer* m_Renderer;
		terrain::Terrain* m_Terrain;
		graphics::MeshFactory m_meshFactory;

		// Some sort of list of entities (tied to models that are in the Renderer (should this be changed to Renderer3D?))
		//Ҫ��Ⱦ��3d�����б�
		std::vector<graphics::Renderable3D*> m_Renderables;

		graphics::Shader terrainShader, modelShader,outlineShader;
	public:
		Scene3D(graphics::FPSCamera* camera, graphics::Window* window);
		~Scene3D();


		void Add(graphics::Renderable3D* renderable);

		void onUpdate(float deltaTime);
		void onRender();

		inline graphics::Renderer* getRenderer() { return m_Renderer; }
		inline graphics::FPSCamera* getCamera() { return m_Camera; }

	private:
		void init();
	};

}