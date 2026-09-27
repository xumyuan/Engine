#pragma once

#include "RenderPass.h"
#include "graphics/Shader.h"
#include "graphics/UniformBufferData.h"
#include "graphics/camera/ICamera.h"
#include "graphics/renderer/RenderTarget.h"

namespace engine {

	// 屏幕空间流体渲染（Screen-Space Fluid Rendering）
	// 深度（球形点精灵）→ 厚度（加法混合）→ 双边滤波平滑深度 → 重建法线并与场景颜色合成
	// 放在延迟光照之后、后处理之前；场景没有流体时原样返回输入
	class FluidRenderPass : public RenderPass {
	public:
		struct Settings {
			int smoothIterations = 3;              // 每轮包含水平 + 竖直各一次滤波
			float blurWorldRadius = 2.0f;          // 世界单位，按深度换算成像素半径
			float maxBlurRadiusPx = 24.0f;
			float blurDepthFalloff = 1.0f;         // 深度差每世界单位的衰减，越大边缘越锐利
			float thicknessScale = 0.3f;
			glm::vec3 absorption = glm::vec3(0.6f, 0.2f, 0.08f);  // 红光吸收最多，深处偏蓝绿
			float refractionStrength = 0.03f;      // 屏幕 UV 偏移量
			float fresnelF0 = 0.02f;               // 水的法向反射率
			float specularPower = 256.0f;
		};

		FluidRenderPass(const RenderScene& renderScene);
		~FluidRenderPass() override;

		LightingPassOutput executeRenderPass(const LightingPassOutput& sceneInput,
			const GeometryPassOutput& gbuffer, ICamera* camera);

		Settings& getSettings() { return m_Settings; }

	private:
		void renderDepth(Texture* sceneDepth);
		void renderThickness(Texture* sceneDepth);
		Texture* smoothDepth();
		void composite(const LightingPassOutput& sceneInput, Texture* fluidDepth);

		void uploadParams(glm::ivec2 blurDirection);

		RenderTarget m_DepthRT;       // R32F 线性距离，0 表示无流体
		RenderTarget m_ThicknessRT;   // R16F
		RenderTarget m_SmoothRTA;     // R32F 双边滤波 ping-pong
		RenderTarget m_SmoothRTB;
		RenderTarget m_CompositeRT;   // RGBA16F，作为后处理的输入

		Shader* m_DepthShader;
		Shader* m_ThicknessShader;
		Shader* m_SmoothShader;
		Shader* m_CompositeShader;

		Settings m_Settings;
	};

}
