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
			// �������� RenderTarget �Ѱ󶨣��������к�Ĭ����Ļ RenderTarget �Ѱ�
			void postLightingPostProcess(opengl::RenderTarget* input);

			// ���������Ҫ���и����Զ�����ڴ������ܻ�����á� Unity����������
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