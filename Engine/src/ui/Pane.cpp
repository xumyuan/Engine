#include "Pane.h"

namespace engine {
	namespace ui {

		Pane::Pane(const std::string& paneName, const glm::vec2& panePositon) : m_PaneName(paneName), m_PanePosition(panePositon) {
		}

		void Pane::render() {
			ImGui::SetWindowPos(ImVec2(m_PanePosition.x, m_PanePosition.y));
			ImGui::Begin(m_PaneName.c_str());
			setupPaneObjects();
			ImGui::End();
		}

	}
}