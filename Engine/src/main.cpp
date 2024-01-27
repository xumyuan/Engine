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

	//��������
	engine::Scene3D scene(&camera, &window);
	engine::graphics::GLCache* glCache = engine::graphics::GLCache::getInstance();

	// ����֡����
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

	engine::Timer fpsTimer;
	int frames = 0;

	engine::Time deltaTime;

	bool firstMove = true;
	GLfloat lastX = window.getMouseX();
	GLfloat lastY = window.getMouseY();

	while (!window.closed()) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // ��������ɫ

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

		//���Ƶ��Զ�����ز���������
		framebuffer.bind();
		window.clear();

		scene.onUpdate(deltaTime.getDeltaTime());
		scene.onRender();

		// �����ز���������blit���Ƕ��ز���������
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.getFramebuffer());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitFramebuffer.getFramebuffer());
		glBlitFramebuffer(0, 0, window.getWidth(), window.getHeight(), 0, 0, window.getWidth(), window.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// ���Ƶ�Ĭ�ϻ�����
		framebuffer.unbind();
		glDisable(GL_BLEND);
		window.clear();
		glCache->switchShader(framebufferShader.getShaderID());
		screenQuad.getMaterial().BindMaterialInformation(framebufferShader);
		screenQuad.Draw();

		window.update();
		if (fpsTimer.elapsed() >= 1) {
			std::cout << "FPS: " << frames << "\n";
			std::cout << "AVG Frame Time: " << (1.0 / frames) * 1000.0 << "ms \n";
			frames = 0;
			fpsTimer.reset();
		}
		else {
			frames++;
		}
	}

	return 0;
}

