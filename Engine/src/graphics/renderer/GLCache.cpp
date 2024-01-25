#include "GLCache.h"

namespace engine {
	namespace graphics {

		GLCache::GLCache() {

		}

		GLCache::~GLCache() {

		}

		GLCache* GLCache::getInstance() {
			static GLCache cache;
			return &cache;
		}

		void GLCache::setDepthTest(bool choice) {
			if (m_DepthTest != choice) {
				m_DepthTest = choice;
				if (m_DepthTest)
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);
			}
		}

		void GLCache::setBlend(bool choice) {
			if (m_Blend != choice) {
				m_Blend = choice;
				if (m_Blend)
					glEnable(GL_BLEND);
				else
					glDisable(GL_BLEND);
			}
		}

		void GLCache::setCull(bool choice) {
			if (m_Cull != choice) {
				m_Cull = choice;
				if (m_Cull)
					glEnable(GL_CULL_FACE);
				else
					glDisable(GL_CULL_FACE);
			}
		}

		void GLCache::setBlendFunc(GLenum src, GLenum dst) {
			if (m_BlendSrc != src || m_BlendDst != dst) {
				m_BlendSrc = src;
				m_BlendDst = dst;
				glBlendFunc(m_BlendSrc, m_BlendDst);
			}
		}

		void GLCache::setDepthFunc(GLenum depthFunc) {
			if (m_DepthFunc != depthFunc) {
				m_DepthFunc = depthFunc;
				glDepthFunc(m_DepthFunc);
			}
		}

		void GLCache::setCullFace(GLenum faceToCull) {
			if (m_FaceToCull != faceToCull) {
				m_FaceToCull = faceToCull;
				glCullFace(m_FaceToCull);
			}
		}

		void GLCache::switchShader(GLuint shaderID) {
			if (m_ActiveShaderID != shaderID) {
				m_ActiveShaderID = shaderID;
				glUseProgram(shaderID);
			}
		}

	}
}