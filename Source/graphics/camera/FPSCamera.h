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
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float SPEED = 40.0f;
	const float SENSITIVITY = 0.1f;
	const float FOV = 80.0f;

	class FPSCamera :public ICamera {
	private:
		// Camera Attributes
		glm::vec3 m_Position, m_Front, m_Up, m_Right, m_WorldUp;
		// Euler Angles
		float m_Yaw;
		float m_Pitch;
		// Camera Options
		float m_MovementSpeed;
		float m_MouseSensitivity;
		float m_FOV;
	public:
		// Vector Constuctor
		FPSCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
		// Scalar Constructor
		FPSCamera(float xPos, float yPos, float zPos, float xUp, float yUp, float zUp, float yaw, float pitch);

		virtual glm::mat4 getViewMatrix() override;
		virtual glm::mat4 getProjectionMatrix() override;
		void processInput(float deltaTime);

		void processKeyboard(Camera_Movement direction, float deltaTime);
		void processMouseMovement(float xOffset, float yOffset, bool constrainPitch);
		void processMouseScroll(float yOffset);

		// Getters
		inline float getYaw() const { return m_Yaw; }
		inline float getPitch() const { return m_Pitch; }
		inline float getMovementSpeed() const { return m_MovementSpeed; }
		inline float getMouseSensitivity() const { return m_MouseSensitivity; }
		inline float getFOV() const { return m_FOV; }
		inline virtual const glm::vec3& getPosition() const override { return m_Position; }
		inline virtual const glm::vec3& getFront()  const override { return m_Front; }
		inline virtual const glm::vec3& getUp() const override { return m_Up; }
	private:
		void updateCameraVectors();
	};

}

