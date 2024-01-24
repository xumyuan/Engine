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
#include "graphics\Model.h"
#include "terrain\Terrain.h"
#include "Scene3D.h"
#include "platform/OpenGL/Framebuffer.h"
#include "graphics/MeshFactory.h"

GLfloat yaw = -90.0f;
GLfloat pitch = 0.0f;

int main() {
	engine::graphics::Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	engine::graphics::Window window("Engine", 1366, 768);
	
	//创建场景
	engine::Scene3D scene(&camera, &window);

	engine::opengl::Framebuffer framebuffer(window.getWidth(), window.getHeight());
	engine::opengl::Framebuffer blitFramebuffer(window.getWidth(), window.getHeight(), false);

	engine::graphics::Shader framebufferShader("src/shaders/framebuffer.vert", "src/shaders/framebuffer.frag");

	engine::graphics::MeshFactory meshFactory;
	engine::graphics::Mesh* colorBufferMesh = meshFactory.CreateScreenQuad(blitFramebuffer.getColorBufferTexture());

	glEnable(GL_DEPTH_TEST);

	engine::Timer fpsTimer;
	int frames = 0;

	framebufferShader.enable();
	framebufferShader.setUniform2f("readOffset", glm::vec2(1.0f / window.getWidth(), 1.0f / window.getHeight()));

	engine::Time deltaTime;

	bool firstMove = true;
	GLfloat lastX = window.getMouseX();
	GLfloat lastY = window.getMouseY();

	while (!window.closed()) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // 场景背景色

		window.clear();
		deltaTime.update();

		// Check to see if the mouse hasn't been moved yet
		if (firstMove && (lastX != window.getMouseX() || lastY != window.getMouseY())) {
			lastX = window.getMouseX();
			lastY = window.getMouseY();
			firstMove = false;
		}

		camera.processMouseMovement(window.getMouseX() - lastX, lastY - window.getMouseY(), true);
		lastX = window.getMouseX();
		lastY = window.getMouseY();

		if (window.isKeyPressed(GLFW_KEY_ESCAPE))
			window.close();

		if (window.isKeyPressed(GLFW_KEY_W))
			camera.processKeyboard(engine::graphics::FORWARD, deltaTime.getDeltaTime());
		if (window.isKeyPressed(GLFW_KEY_S))
			camera.processKeyboard(engine::graphics::BACKWARD, deltaTime.getDeltaTime());
		if (window.isKeyPressed(GLFW_KEY_A))
			camera.processKeyboard(engine::graphics::LEFT, deltaTime.getDeltaTime());
		if (window.isKeyPressed(GLFW_KEY_D))
			camera.processKeyboard(engine::graphics::RIGHT, deltaTime.getDeltaTime());
		if (window.isKeyPressed(GLFW_KEY_SPACE))
			camera.processKeyboard(engine::graphics::UPWARDS, deltaTime.getDeltaTime());
		if (window.isKeyPressed(GLFW_KEY_LEFT_CONTROL))
			camera.processKeyboard(engine::graphics::DOWNWARDS, deltaTime.getDeltaTime());

		camera.processMouseScroll(window.getScrollY() * 6);
		window.resetScroll();

		//绘制到自定义多重采样缓冲区
		framebuffer.bind();
		window.clear();

		scene.onUpdate(deltaTime.getDeltaTime());
		scene.onRender();

		// 将多重采样缓冲区blit到非多重采样缓冲区
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.getFramebuffer());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitFramebuffer.getFramebuffer());
		glBlitFramebuffer(0, 0, window.getWidth(), window.getHeight(), 0, 0, window.getWidth(), window.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// 绘制到默认缓冲区
		framebuffer.unbind();
		glDisable(GL_BLEND);
		window.clear();
		framebufferShader.enable();
		colorBufferMesh->Draw(framebufferShader);
		framebufferShader.disable();

		window.update();
		if (fpsTimer.elapsed() >= 1) {
			std::cout << "FPS: " << frames << std::endl;
			frames = 0;
			fpsTimer.reset();
		}
		else {
			frames++;
		}
	}

	return 0;
}

