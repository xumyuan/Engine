#pragma once

#include "imgui.h"

namespace engine {
	namespace ui {

		class Pane {
		public:
			Pane(const std::string& paneName, const glm::vec2& paneSize);

			void render();
		protected:
			virtual void setupPaneObjects() = 0;

			std::string m_PaneName;
			glm::vec2 m_PaneSize;
		};

	}
}