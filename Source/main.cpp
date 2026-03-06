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

#include <filesystem>
#include <string>

// RHI 测试前向声明
namespace engine::rhi { void testNullDevice(); }

int main(int argc, char* argv[]) {
	// --test-rhi: 仅运行 RHI NullDevice 测试后退出
	if (argc > 1 && std::string(argv[1]) == "--test-rhi") {
		engine::rhi::testNullDevice();
		return 0;
	}

	// 使用 CMake 注入的项目根目录，确保无论从哪里启动 exe 都能找到资源
	std::filesystem::current_path(PROJECT_ROOT_DIR);

	spdlog::info("window create...");
	engine::Window window("Engine", WINDOW_X_RESOLUTION, WINDOW_Y_RESOLUTION);
	spdlog::info("window create succeed!");

	spdlog::info("load textures...");
	engine::TextureLoader::initializeDefaultTextures();
	spdlog::info("load textures over.");

	//创建场景
	spdlog::info("create scene...");
	engine::Scene3D scene(&window);
	spdlog::info("create scene over.");


	engine::MasterRenderer renderer(&scene);

	// 准备ui
	engine::RuntimePane runtimePane(glm::vec2(256.0f, 90.0f));
	engine::DebugPane debugPane(glm::vec2(256.0f, 115.0f));
	renderer.init();

	window.show();
#if DEBUG_ENABLED
	engine::Timer timer;
#endif

	engine::Time deltaTime;


	while (!window.closed()) {
		deltaTime.update();

#if DEBUG_ENABLED
		//线框模式
		if (debugPane.getWireframeMode())
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

		window.bind();
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

		if (engine::InputManager::isKeyPressed(GLFW_KEY_C)) {
			auto* camera = scene.getCamera();
			glm::vec3 pos = camera->getPosition();
			spdlog::info("Camera Position: ({}, {}, {})", pos.x, pos.y, pos.z);
		}

		scene.onUpdate(deltaTime.getDeltaTime());
		renderer.render();


		// 展示ui面板
		runtimePane.render();
		debugPane.render();


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		window.update();
	}

	return 0;
}

