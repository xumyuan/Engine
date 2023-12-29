#include "FPSCamera.h"

namespace engine {
	namespace graphics {

		FPSCamera::FPSCamera(glm::vec3 position, glm::vec3 up, GLfloat yaw, GLfloat pitch)
			: m_Front(glm::vec3(0.0f, 0.0f, -1.0f)), m_MovementSpeed(SPEED), m_MouseSensitivity(SENSITIVITY), m_FOV(FOV)
		{
			m_Position = position;
			m_Up = up;
			m_Yaw = yaw;
			m_Pitch = pitch;
			updateCameraVectors();
		}

		FPSCamera::FPSCamera(GLfloat xPos, GLfloat yPos, GLfloat zPos, GLfloat xUp, GLfloat yUp, GLfloat zUp, GLfloat yaw, GLfloat pitch)
			: m_Front(glm::vec3(0.0f, 0.0f, -1.0f)), m_MovementSpeed(SPEED), m_MouseSensitivity(SENSITIVITY), m_FOV(FOV)
		{
			m_Position = glm::vec3(xPos, yPos, zPos);
			m_Up = glm::vec3(xUp, yUp, zUp);
			m_Yaw = yaw;
			m_Pitch = pitch;
			updateCameraVectors();
		}

		glm::mat4 FPSCamera::getViewMatrix() {
			return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		}

		void FPSCamera::processKeyboard(Camera_Movement direction, GLfloat deltaTime) {
			GLfloat velocity = m_MovementSpeed * deltaTime;
			if (direction == FORWARD) {
				m_Position += m_Front * velocity;
			}
			if (direction == BACKWARD) {
				m_Position -= m_Front * velocity;
			}
			if (direction == LEFT) {
				m_Position -= m_Right * velocity;
			}
			if (direction == RIGHT) {
				m_Position += m_Right * velocity;
			}
		}

		void FPSCamera::processMouseMovement(GLfloat xOffset, GLfloat yOffset, GLboolean constrainPitch) {
			xOffset *= m_MouseSensitivity;
			yOffset *= m_MouseSensitivity;

			m_Yaw += xOffset;
			m_Pitch += yOffset;

			// Constrain the pitch
			if (constrainPitch) {
				if (m_Pitch > 89.0f) {
					m_Pitch = 89.0f;
				}
				else if (m_Pitch < -89.0f) {
					m_Pitch = -89.0f;
				}
			}

			updateCameraVectors();
		}

		void FPSCamera::processMouseScroll(GLfloat yOffset) {
			if (m_FOV >= 1.0f && m_FOV <= FOV) {
				m_FOV -= yOffset;
			}
			if (m_FOV < 1.0f) {
				m_FOV = 1.0f;
			}
			else if (m_FOV > FOV) {
				m_FOV = FOV;
			}
		}

		void FPSCamera::updateCameraVectors() {
			glm::vec3 front;
			front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
			front.y = sin(glm::radians(m_Pitch));
			front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
			m_Front = glm::normalize(front);

			// Recalculate Vectors
			m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
			m_Up = glm::normalize(glm::cross(m_Right, m_Front));
		}

	}
}