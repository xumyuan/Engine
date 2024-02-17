#pragma once

#include "MeshRenderer.h"

#include "graphics/Shader.h"
#include "graphics/mesh/common/Quad.h"
#include "platform/OpenGL/Framebuffers/RenderTarget.h"
#include "ui/DebugPane.h"
#include "ui/RuntimePane.h"
#include "utils/Timer.h"

namespace engine {
	namespace graphics {

		class PostProcessor {
		public:
			PostProcessor(MeshRenderer* renderer);
			~PostProcessor();

			void preLightingPostProcess();
			// 假设输入 RenderTarget 已绑定，函数运行后默认屏幕 RenderTarget 已绑定
			void postLightingPostProcess(opengl::RenderTarget* input);

			// 如果我们想要进行更多自定义后期处理，可能会很有用。 Unity是这样做的
			//void blit(Texture *texture, opengl::RenderTarget *source);

			inline void EnableBlur(bool choice) { m_Blur = choice; }
		private:
			float m_GammaCorrection = 2.2f;

			MeshRenderer* m_MeshRenderer;
			Shader m_PostProcessShader;
			Quad m_NDC_Plane;
			opengl::RenderTarget m_ScreenRenderTarget;
			Timer m_Timer;

			bool m_Blur = false;
		};

	}
}