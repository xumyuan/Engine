#include "pch.h"
#include "scene/Scene3D.h"

#include "graphics/Shader.h"
#include "graphics/Window.h"
#include "graphics/camera/FPSCamera.h"
#include "graphics/mesh/Model.h"
#include "graphics/mesh/common/Quad.h"
#include "graphics/renderer/GLCache.h"
#include "graphics/renderer/MasterRenderer.h"
#include "input/InputManager.h"
#include "platform/OpenGL/Framebuffers/Framebuffer.h"
#include "terrain/Terrain.h"
#include "ui/DebugPane.h"
#include "ui/RuntimePane.h"
#include "utils/Time.h"
#include "utils/Timer.h"

int main() {

	engine::Window window("Engine", WINDOW_X_RESOLUTION, WINDOW_Y_RESOLUTION);
	engine::TextureLoader::initializeDefaultTextures();

	//创建场景
	engine::Scene3D scene(&window);
	engine::MasterRenderer renderer(&scene);

	// 准备ui
	engine::RuntimePane runtimePane(glm::vec2(256.0f, 90.0f));
	engine::DebugPane debugPane(glm::vec2(256.0f, 115.0f));

#if DEBUG_ENABLED
	engine::Timer timer;
#endif

	engine::Time deltaTime;
	while (!window.closed()) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // 场景背景色
		deltaTime.update();

#if DEBUG_ENABLED
		if (debugPane.getWireframeMode())
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
		window.clear();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
#if DEBUG_ENABLED
		glFinish();
		timer.reset();
#endif
		if (engine::InputManager::isKeyPressed(GLFW_KEY_ESCAPE))
			window.close();
		scene.onUpdate(deltaTime.getDeltaTime());
		renderer.render();

		// Display panes
		runtimePane.render();
		debugPane.render();


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		window.update();
	}

	return 0;
}

