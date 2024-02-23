#include "pch.h"
#include "scene/Scene3D.h"

#include "graphics/Shader.h"
#include "graphics/Window.h"
#include "graphics/camera/FPSCamera.h"
#include "graphics/mesh/Model.h"
#include "graphics/mesh/common/Quad.h"
#include "graphics/renderer/GLCache.h"
#include "graphics/renderer/PostProcessor.h"
#include "input/InputManager.h"
#include "platform/OpenGL/Framebuffers/Framebuffer.h"
#include "terrain/Terrain.h"
#include "ui/DebugPane.h"
#include "ui/RuntimePane.h"
#include "utils/Time.h"
#include "utils/Timer.h"

int main() {

	engine::Window window("Engine", WINDOW_X_RESOLUTION, WINDOW_Y_RESOLUTION);

	//创建场景
	engine::Scene3D scene(&window);
	engine::GLCache* glCache = engine::GLCache::getInstance();
	engine::TextureLoader::initializeDefaultTextures();
	engine::PostProcessor postProcessor(scene.getModelRenderer());

	// 准备ui
	engine::RuntimePane runtimePane(glm::vec2(256.0f, 90.0f));
	engine::DebugPane debugPane(glm::vec2(256.0f, 115.0f));

	// 创建帧缓冲
	bool shouldMultisample = MSAA_SAMPLE_AMOUNT > 1.0 ? true : false;
	engine::Framebuffer framebuffer(window.getWidth(), window.getHeight());
	framebuffer.addTexture2DColorAttachment(shouldMultisample).addDepthStencilRBO(shouldMultisample).createFramebuffer();

	// TODO: MAKE MULTISAMPLE OPTION WORK OR INVESTIGATE
	engine::Framebuffer shadowmap(SHADOWMAP_RESOLUTION_X, SHADOWMAP_RESOLUTION_Y);
	shadowmap.addDepthAttachment(false).createFramebuffer();


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
		// Shadowmap Pass
		glViewport(0, 0, shadowmap.getWidth(), shadowmap.getHeight());
		shadowmap.bind();
		shadowmap.clear();
		scene.shadowmapPass();
#if DEBUG_ENABLED
		glFinish();
		runtimePane.setShadowmapTimer(timer.elapsed());
#endif


		if (engine::InputManager::isKeyPressed(GLFW_KEY_ESCAPE))
			window.close();


		//绘制到自定义多重采样缓冲区
		glViewport(0, 0, framebuffer.getWidth(), framebuffer.getHeight());

		framebuffer.bind();
		framebuffer.clear();
		scene.onUpdate(deltaTime.getDeltaTime());
		scene.onRender(shadowmap.getDepthTexture());


#if DEBUG_ENABLED
		glFinish();
		timer.reset();
#endif
		// Peform post processing
		postProcessor.postLightingPostProcess(&framebuffer);

		// Display panes
		runtimePane.render();
		debugPane.render();


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		window.update();
	}

	return 0;
}

