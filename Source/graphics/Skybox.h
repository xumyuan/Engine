#pragma once
#include "Shader.h"
#include "Window.h"

#include "graphics/camera/FPSCamera.h"
#include "rhi/include/RHIDevice.h"
#include "rhi/include/RHIResources.h"
#include "rhi/include/RHICommandBuffer.h"
#include "utils/loaders/TextureLoader.h"

namespace engine {

	class Skybox {
	public:
		Skybox(const std::vector<std::string>& filePaths);
		~Skybox();

		void Draw(ICamera* camera);
		void Draw(rhi::CommandBuffer& cmd, ICamera* camera);

		Cubemap* getSkyboxCubemap() { return m_SkyboxCubemap; }
	private:
		Shader *m_SkyboxShader;

		rhi::RHIDevice*            m_Device = nullptr;
		rhi::BufferHandle          m_VertexBuffer;
		rhi::BufferHandle          m_IndexBuffer;
		rhi::RenderPrimitiveHandle m_RenderPrimitive;
		Cubemap* m_SkyboxCubemap;
	};

}