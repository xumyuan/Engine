#pragma once
#include "Shader.h"
#include "Window.h"

#include "graphics/camera/FPSCamera.h"
#include "graphics/renderer/GLCache.h"
#include "rhi/include/RHIDevice.h"
#include "utils/loaders/TextureLoader.h"

namespace engine {

	class Skybox {
	public:
		Skybox(const std::vector<std::string>& filePaths);
		~Skybox();

		void Draw(ICamera* camera);

		Cubemap* getSkyboxCubemap() { return m_SkyboxCubemap; }
	private:
		Shader *m_SkyboxShader;
		GLCache* m_GLCache;

		rhi::RHIDevice*            m_Device = nullptr;
		rhi::BufferHandle          m_VertexBuffer;
		rhi::BufferHandle          m_IndexBuffer;
		rhi::RenderPrimitiveHandle m_RenderPrimitive;
		Cubemap* m_SkyboxCubemap;
	};

}