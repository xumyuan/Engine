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
#include "graphics\camera\FPSCamera.h"
#include "utils\Logger.h"
#include "graphics\Model.h"
#include "terrain\Terrain.h"
#include "Scene3D.h"





GLfloat yaw = -90.0f;
GLfloat pitch = 0.0f;

int main() {
	engine::graphics::FPSCamera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	engine::graphics::Window window("Engine", 1366, 768);
	engine::Scene3D scene(&camera, &window);

	glEnable(GL_DEPTH_TEST);

	engine::Timer fpsTimer;
	int frames = 0;

	engine::Time deltaTime;


	bool firstMove = true;
	GLfloat lastX = window.getMouseX();
	GLfloat lastY = window.getMouseY();


	while (!window.closed()) {
		glClearColor(0.2f, 0.f, 0.0f, 1.0f);  // ³¡¾°±³¾°É«

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

		scene.onUpdate(deltaTime.getDeltaTime());
		scene.onRender();
	
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

