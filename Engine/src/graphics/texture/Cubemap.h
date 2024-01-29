#pragma once

#include <gl/glew.h>
#include "../../Defs.h"

namespace engine {
	namespace graphics {

		class Cubemap {
		public:
			Cubemap();
			~Cubemap();

			void generateCubemapFace(GLenum face, unsigned int faceWidth, unsigned int faceHeight, GLenum textureFormat, GLenum dataFormat, const unsigned char* data);

			void bind(int unit = -1);
			void unbind();
		private:

			// TODO:�о����õĹ��ˣ������������֧�֣����о���������ͼ���ʵ� mips
			unsigned int m_CubemapId;

			unsigned int m_FaceWidth, m_FaceHeight;
			GLenum m_TextureFormat;

			// Texture wrapping options
			GLenum m_TextureWrapSMode = GL_CLAMP_TO_EDGE;
			GLenum m_TextureWrapTMode = GL_CLAMP_TO_EDGE;
			GLenum m_TextureWrapRMode = GL_CLAMP_TO_EDGE;

			// Texture filtering options
			GLenum m_TextureMinificationFilterMode = GL_LINEAR; // �������Ƶø�Զ���Ҷ������ӳ�䵽һ������ʱ�Ĺ���ģʽ���������Ի�����������
			GLenum m_TextureMagnificationFilterMode = GL_LINEAR; // �������ø��ӽ����Ҷ������ӳ�䵽��������ʱ�Ĺ���ģʽ����Զ����Ҫ����˫���ԣ���Ϊ���������������һ��׼ȷ��

		};
	}
}