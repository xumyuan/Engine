#pragma once

#include "ModelRenderer.h"

#include "graphics/Shader.h"
#include "graphics/mesh/common/Quad.h"
#include "platform/OpenGL/Framebuffers/Framebuffer.h"
#include "ui/DebugPane.h"
#include "ui/RuntimePane.h"
#include "utils/Timer.h"

namespace engine {

	class PostProcessor {
	public:
		PostProcessor(ModelRenderer* renderer);
		~PostProcessor();

		void preLightingPostProcess();
		// �������� RenderTarget �Ѱ󶨣��������к�Ĭ����Ļ RenderTarget �Ѱ�
		void postLightingPostProcess(Framebuffer* input);

		// ���������Ҫ���и����Զ�����ڴ������ܻ�����á� Unity����������
		//void blit(Texture *texture, RenderTarget *source);

		inline void EnableBlur(bool choice) { m_Blur = choice; }
	private:
		float m_GammaCorrection = 2.2f;

		ModelRenderer* m_MeshRenderer;
		Shader m_PostProcessShader;
		Quad m_NDC_Plane;
		Framebuffer m_ScreenRenderTarget;
		Timer m_Timer;

		bool m_Blur = false;
	};

}