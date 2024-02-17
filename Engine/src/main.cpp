#include "pch.h"
#include "Scene3D.h"

#include "graphics/Shader.h"
#include "graphics/Window.h"
#include "graphics/camera/FPSCamera.h"
#include "graphics/mesh/Model.h"
#include "graphics/mesh/common/Quad.h"
#include "graphics/renderer/GLCache.h"
#include "graphics/renderer/PostProcessor.h"
#include "platform/OpenGL/Framebuffers/RenderTarget.h"
#include "terrain/Terrain.h"
#include "ui/DebugPane.h"
#include "ui/RuntimePane.h"
#include "utils/Time.h"
#include "utils/Timer.h"

GLfloat yaw = -90.0f;
GLfloat pitch = 0.0f;

int main() {
	engine::graphics::FPSCamera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	engine::graphics::Window window("Engine", WINDOW_X_RESOLUTION, WINDOW_Y_RESOLUTION);

	//创建场景
	engine::Scene3D scene(&camera, &window);
	engine::graphics::GLCache* glCache = engine::graphics::GLCache::getInstance();
	engine::utils::TextureLoader::initializeDefaultTextures();
	engine::graphics::PostProcessor postProcessor(scene.getRenderer());

	// 准备ui
	engine::ui::RuntimePane runtimePane(glm::vec2(256.0f, 90.0f));
	engine::ui::DebugPane debugPane(glm::vec2(256.0f, 115.0f));

	// 创建帧缓冲
	bool shouldMultisample = MSAA_SAMPLE_AMOUNT > 1.0 ? true : false;
	engine::opengl::RenderTarget framebuffer(window.getWidth(), window.getHeight());
	framebuffer.addColorAttachment(shouldMultisample).addDepthStencilRBO(shouldMultisample).createFramebuffer();

	// TODO: MAKE MULTISAMPLE OPTION WORK OR INVESTIGATE
	engine::opengl::RenderTarget shadowmap(SHADOWMAP_RESOLUTION_X, SHADOWMAP_RESOLUTION_Y);
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

		camera.processInput(deltaTime.getDeltaTime());

		if (window.isKeyPressed(GLFW_KEY_ESCAPE))
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
		window.resetScroll();
		window.update();
	}

	return 0;
}

