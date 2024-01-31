#pragma once

#include "Pane.h"
#include "../Defs.h"

namespace engine {
	namespace ui {

		class DebugPane : public Pane {
		public:
			DebugPane(const glm::vec2& panePosition);

			virtual void setupPaneObjects();

			inline static bool getWireframeMode() { return s_WireframeMode; }
			inline static void setWireframeMode(bool choice) { s_WireframeMode = choice; }
		private:
			static bool s_WireframeMode;
		};

	}
}