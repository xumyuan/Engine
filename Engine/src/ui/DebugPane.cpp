#include "pch.h"
#include "DebugPane.h"

namespace engine {

	glm::vec3* DebugPane::s_CameraPosition = nullptr;
	bool DebugPane::s_WireframeMode = false;
	float* DebugPane::s_GammaCorrectionValue = nullptr;
	float* DebugPane::s_ExposureValue = nullptr;
	bool* DebugPane::s_FxaaEnabled = nullptr;

	DebugPane::DebugPane(const glm::vec2& panePosition) : Pane(std::string("Debug Controls"), panePosition)
	{
	}

	void DebugPane::setupPaneObjects() {
		if (s_FxaaEnabled != nullptr)
			ImGui::Checkbox("FXAA", s_FxaaEnabled);
		if (s_GammaCorrectionValue != nullptr)
			ImGui::SliderFloat("Gamma", s_GammaCorrectionValue, 0.5f, 3.0f, "%.2f");
		if (s_ExposureValue != nullptr)
			ImGui::SliderFloat("Exposure", s_ExposureValue, 0.1f, 5.0f, "%.2f");
		if (s_CameraPosition != nullptr)
			ImGui::Text("Camera Pos x:%.1f y:%.1f z:%.1f", s_CameraPosition->x, s_CameraPosition->y, s_CameraPosition->z);


#if DEBUG_ENABLED
		ImGui::Text("Hit \"P\" to show/hide the cursor");
		ImGui::Checkbox("Wireframe Mode", &s_WireframeMode);
#endif
	}


}