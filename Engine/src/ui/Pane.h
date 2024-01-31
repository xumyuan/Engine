#pragma once

#include <string>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include "imgui.h"

namespace engine {
	namespace ui {

		class Pane {
		public:
			Pane(const std::string& paneName, const glm::vec2& panePositon);

			void render();
		protected:
			virtual void setupPaneObjects() = 0;

			std::string m_PaneName;
			glm::vec2 m_PanePosition;
		};

	}
}