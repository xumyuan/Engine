#pragma once
#include "utils/Singleton.h"

namespace engine {
	// 用于opengl状态管理，单例实现
	class GLCache : Singleton {
	public:
		GLCache();
		~GLCache();

		static GLCache* getInstance();

		void setDepthTest(bool choice);
		void setStencilTest(bool choice);
		void setBlend(bool choice);
		void setFaceCull(bool choice);
		void setMultisample(bool choice);

		void setDepthFunc(GLenum depthFunc);
		void setStencilFunc(GLenum testFunc, GLint stencilFragValue, GLuint stencilBitmask);
		void setStencilOp(GLenum stencilFailOperation, GLenum depthFailOperation, GLenum depthPassOperation);
		void setStencilWriteMask(GLuint bitmask);
		void setBlendFunc(GLenum src, GLenum dst);
		void setCullFace(GLenum faceToCull);

		void switchShader(GLuint shaderID);
	private:
		// Toggles
		bool m_DepthTest;
		bool m_StencilTest;
		bool m_Blend;
		bool m_Cull;
		bool m_Multisample;

		// 深度 State
		GLenum m_DepthFunc;

		// 模板 State
		GLenum m_StencilTestFunc;
		GLint m_StencilFragValue;
		GLuint m_StencilFuncBitmask;

		GLenum m_StencilFailOperation, m_DepthFailOperation, m_DepthPassOperation;
		GLuint m_StencilWriteBitmask;

		// 混合 State
		GLenum m_BlendSrc, m_BlendDst;

		// 背面剔除 State
		GLenum m_FaceToCull;

		// 当前绑定shader
		GLuint m_ActiveShaderID;
	};

}

