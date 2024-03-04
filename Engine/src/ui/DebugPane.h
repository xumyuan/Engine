#pragma once

#include "Pane.h"

namespace engine {

	class DebugPane : public Pane {
	public:
		DebugPane(const glm::vec2& panePosition);

		virtual void setupPaneObjects();

		static inline void bindCameraPositionValue(glm::vec3* ptr) { s_CameraPosition = ptr; }
		static inline void bindGammaCorrectionValue(float* ptr) { s_GammaCorrectionValue = ptr; }
		static inline void bindExposureValue(float* ptr) { s_ExposureValue = ptr; }
		static inline bool getWireframeMode() { return s_WireframeMode; }
		static inline void setWireframeMode(bool choice) { s_WireframeMode = choice; }
	private:
		static glm::vec3* s_CameraPosition;
		// gamma ����
		static float* s_GammaCorrectionValue;
		static float* s_ExposureValue;

		static bool s_WireframeMode;
	};


}