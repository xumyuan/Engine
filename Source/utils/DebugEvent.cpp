#include "pch.h"
#include "DebugEvent.h"

namespace engine {
	// 设置调试组标记（兼容不同版本）
	void pushDebugGroup(const std::string& name) {
		if (GLEW_VERSION_4_3 || glewIsSupported("GL_KHR_debug")) {
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
		}
#ifdef GL_GREMEDY_string_marker
		else if (glewIsSupported("GL_GREMEDY_string_marker")) {
			glStringMarkerGREMEDY(0, name.c_str());
		}
#endif
	}

	void popDebugGroup() {
		if (GLEW_VERSION_4_3 || glewIsSupported("GL_KHR_debug")) {
			glPopDebugGroup();
		}
	}
}
