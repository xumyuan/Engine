#pragma once

#include <graphics/Shader.h>
#include <graphics/texture/Texture.h>
#include <graphics/texture/Cubemap.h>

namespace engine {

	class ReflectionProbe {
	public:
		ReflectionProbe(glm::vec3& probePosition, glm::vec2& probeResolution, bool isStatic);
		~ReflectionProbe();

		void generate();

		// Assumes the shader is bound
		void bind(Shader* shader);

		// Getters
		inline Cubemap* getPrefilterMap() { return m_PrefilterMap; }
		static inline Texture* getBRDFLUT() { return s_BRDF_LUT; }

		// Setters
		static void setBRDFLUT(Texture* texture) { s_BRDF_LUT = texture; }
	private:
		Cubemap* m_PrefilterMap;
		static Texture* s_BRDF_LUT;

		glm::vec3 m_Position;
		glm::vec2 m_ProbeResolution;
		bool m_IsStatic;
		bool m_Generated;
	};

}