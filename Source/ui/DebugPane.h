#pragma once

#include "Pane.h"

#include <functional>

namespace engine {

	class DebugPane : public Pane {
	public:
		using DrawFn = std::function<void()>;

		DebugPane(const glm::vec2& panePosition);

		virtual void setupPaneObjects();

		// 各系统自己画自己的控件。order 越小越靠前（建议：10 相机，20 场景，30 光照，40 后处理）。
		// 回调在 removeSection 之前必须保持有效。
		static int addSection(const char* name, DrawFn draw, int order = 0, bool openByDefault = false);
		static void removeSection(int id);

		static inline bool getWireframeMode() { return s_WireframeMode; }
		static inline bool getLightMarkersEnabled() { return s_LightMarkersEnabled; }
		static inline void setWireframeMode(bool choice) { s_WireframeMode = choice; }
	private:
		static bool s_WireframeMode;
		static bool s_LightMarkersEnabled;
	};

}
