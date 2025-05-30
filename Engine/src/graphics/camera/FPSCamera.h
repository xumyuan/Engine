#pragma once

#include "graphics/Window.h"
#include "ui/DebugPane.h"
#include "graphics/camera/ICamera.h"

namespace engine {

	enum Camera_Movement {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UPWARDS,
		DOWNWARDS
	};

	// Default Camera Values
	const GLfloat YAW = -90.0f;
	const GLfloat PITCH = 0.0f;
	const GLfloat SPEED = 40.0f;
	const GLfloat SENSITIVITY = 0.1f;
	const GLfloat FOV = 80.0f;

	class FPSCamera :public ICamera {
	private:
		// Camera Attributes
		glm::vec3 m_Position, m_Front, m_Up, m_Right, m_WorldUp;
		// Euler Angles
		GLfloat m_Yaw;
		GLfloat m_Pitch;
		// Camera Options
		GLfloat m_MovementSpeed;
		GLfloat m_MouseSensitivity;
		GLfloat m_FOV;
	public:
		// Vector Constuctor
		FPSCamera(glm::vec3 position, glm::vec3 up, GLfloat yaw, GLfloat pitch);
		// Scalar Constructor
		FPSCamera(GLfloat xPos, GLfloat yPos, GLfloat zPos, GLfloat xUp, GLfloat yUp, GLfloat zUp, GLfloat yaw, GLfloat pitch);

		virtual glm::mat4 getViewMatrix() override;
		virtual glm::mat4 getProjectionMatrix() override;
		void processInput(GLfloat deltaTime);

		void processKeyboard(Camera_Movement direction, GLfloat deltaTime);
		void processMouseMovement(GLfloat xOffset, GLfloat yOffset, GLboolean constrainPitch);
		void processMouseScroll(GLfloat yOffset);

		// Getters
		inline GLfloat getYaw() const { return m_Yaw; }
		inline GLfloat getPitch() const { return m_Pitch; }
		inline GLfloat getMovementSpeed() const { return m_MovementSpeed; }
		inline GLfloat getMouseSensitivity() const { return m_MouseSensitivity; }
		inline GLfloat getFOV() const { return m_FOV; }
		inline virtual const glm::vec3& getPosition() const override { return m_Position; }
		inline virtual const glm::vec3& getFront() const { return m_Front; }
		inline virtual const glm::vec3& getUp() const { return m_Up; }
	private:
		void updateCameraVectors();
	};

}

