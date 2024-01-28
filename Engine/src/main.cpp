#include <iostream>
#include "graphics\Window.h"
#include "utils\Timer.h"
#include "graphics\Shader.h"
#include <SOIL/SOIL.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils\Time.h"
#include "graphics\camera\Camera.h"
#include "utils\Logger.h"
#include "graphics\mesh\Model.h"
#include "terrain\Terrain.h"
#include "Scene3D.h"
#include "platform/OpenGL/Framebuffers/Framebuffer.h"
#include "graphics/mesh/common/Quad.h"
#include "graphics/renderer/GLCache.h"

GLfloat yaw = -90.0f;
GLfloat pitch = 0.0f;

int main() {
	engine::graphics::Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	engine::graphics::Window window("Engine", WINDOW_X_RESOLUTION, WINDOW_Y_RESOLUTION);

	//创建场景
	engine::Scene3D scene(&camera, &window);
	engine::graphics::GLCache* glCache = engine::graphics::GLCache::getInstance();

	// 创建帧缓冲
	engine::opengl::Framebuffer framebuffer(window.getWidth(), window.getHeight());
	framebuffer.addColorAttachment(true).addDepthStencilRBO(true).createFramebuffer();

	engine::opengl::Framebuffer blitFramebuffer(window.getWidth(), window.getHeight());
	blitFramebuffer.addColorAttachment(false).addDepthStencilRBO(false).createFramebuffer();

	engine::graphics::Shader framebufferShader("src/shaders/postprocess.vert", "src/shaders/postprocess.frag");

	engine::graphics::Quad screenQuad;
	screenQuad.getMaterial().setDiffuseMapId(blitFramebuffer.getColorBufferTexture());

	// Setup post processing information
	glCache->switchShader(framebufferShader.getShaderID());
	framebufferShader.setUniform2f("readOffset", glm::vec2(1.0f / (float)window.getWidth(), 1.0f / (float)window.getHeight()));

	glEnable(GL_DEPTH_TEST);
#if DEBUG_ENABLED
	engine::Timer timer;
	float postProcessTime = 0.0f;
#endif

	engine::Time deltaTime;

	bool firstMove = true;
	GLfloat lastX = window.getMouseX();
	GLfloat lastY = window.getMouseY();

	while (!window.closed()) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // 场景背景色

		window.clear();
		deltaTime.update();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		camera.processInput(deltaTime.getDeltaTime());

		if (window.isKeyPressed(GLFW_KEY_ESCAPE))
			window.close();

		//绘制到自定义多重采样缓冲区
		framebuffer.bind();
		window.clear();
		scene.onUpdate(deltaTime.getDeltaTime());
		scene.onRender();

#if DEBUG_ENABLED
		glFinish();
		timer.reset();
#endif

		// 将多重采样缓冲区blit到非多重采样缓冲区
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.getFramebuffer());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitFramebuffer.getFramebuffer());
		glBlitFramebuffer(0, 0, window.getWidth(), window.getHeight(), 0, 0, window.getWidth(), window.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		// 绘制到默认缓冲区
		framebuffer.unbind();
		glDisable(GL_BLEND);
		window.clear();
		glCache->switchShader(framebufferShader.getShaderID());
		screenQuad.getMaterial().BindMaterialInformation(framebufferShader);
		screenQuad.Draw();
#if DEBUG_ENABLED
		glFinish();
		postProcessTime = timer.elapsed();
#endif

		{
			ImGui::SetWindowPos(ImVec2(100.f, 50.f));
			ImGui::Begin("Runtime Analytics", nullptr);


			ImGui::Text("Frametime: %.3f ms (FPS %.1f)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
#if DEBUG_ENABLED
			ImGui::Text("Post Process: %.6f ms", 1000.0f * postProcessTime);
#endif
			ImGui::End();
		}
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		window.resetScroll();
		window.update();
	}

	return 0;
}

