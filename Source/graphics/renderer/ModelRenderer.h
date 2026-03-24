#pragma once
#include "graphics/renderer/renderpass/RenderPassType.h"

#include "scene/RenderableModel.h"
#include "graphics/camera/FPSCamera.h"
#include "graphics/mesh/Model.h"
#include <graphics/mesh/common/Cube.h>
#include "graphics/mesh/common/Quad.h"
#include "rhi/include/RHIDevice.h"
#include "rhi/include/RHIResources.h"
#include "rhi/include/RHICommandBuffer.h"


namespace engine {
	class ModelRenderer {
	public:
		ModelRenderer(FPSCamera* camera);

		void submitOpaque(RenderableModel* renderable);
		void submitTransparent(RenderableModel* renderable);

		void flushOpaque(Shader* shader, RenderPassType pass);
		void flushTransparent(Shader* shader, RenderPassType pass);

		// 命令缓冲版本
		void flushOpaque(rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass);
		void flushTransparent(rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass);

		static void drawNdcCube();
		static void drawNdcPlane();
		static void drawNdcCube(rhi::CommandBuffer& cmd);
		static void drawNdcPlane(rhi::CommandBuffer& cmd);
	public:
	 static	Quad* NDC_Plane;
	 static Cube* NDC_Cube;

	private:
		void setupModelMatrix(RenderableModel* renderable, Shader* shader, RenderPassType pass);
		void setupModelMatrix(RenderableModel* renderable, rhi::CommandBuffer& cmd, rhi::ProgramHandle program, RenderPassType pass);


		std::deque<RenderableModel*> m_OpaqueRenderQueue;
		std::deque<RenderableModel*> m_TransparentRenderQueue;

		FPSCamera* m_Camera;
	};

}



