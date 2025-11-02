#pragma once

namespace engine {

	class Poly6Kernel {
	public:
		static float m_radius;
		static float m_k;
		static float m_l;
		static float m_m;
		static float m_W_zero;
	public:
		static float getRadius() { return m_radius; }
		static void setRadius(float val)
		{
			m_radius = val;
			const float pi = glm::pi<float>();

			m_k = static_cast<float>(315.0) / (static_cast<float>(64.0) * pi * pow(m_radius, static_cast<float>(9.0)));
			m_l = -static_cast<float>(945.0) / (static_cast<float>(32.0) * pi * pow(m_radius, static_cast<float>(9.0)));
			m_m = m_l;
			m_W_zero = W(glm::vec3(0.0f));
		}
	public:
		/**
		* W(r,h) = (315/(64 pi h^9))(h^2-|r|^2)^3
		*        = (315/(64 pi h^9))(h^2-r*r)^3
		*/
		static float W(const float r)
		{
			float res = 0.0;
			const float r2 = r * r;
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				res = pow(radius2 - r2, static_cast<float>(3.0)) * m_k;
			}
			return res;
		}

		static float W(const glm::vec3& r)
		{
			float res = 0.0;
			const float r2 = glm::length2(r);
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				res = pow(radius2 - r2, static_cast<float>(3.0)) * m_k;
			}
			return res;
		}

		/**
		* grad(W(r,h)) = r(-945/(32 pi h^9))(h^2-|r|^2)^2
		*              = r(-945/(32 pi h^9))(h^2-r*r)^2
		*/
		static glm::vec3 gradW(const glm::vec3& r)
		{
			glm::vec3 res;
			const float r2 = glm::length2(r);
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				float tmp = radius2 - r2;
				res = m_l * tmp * tmp * r;
			}
			else
				res = glm::vec3(0.0f);

			return res;
		}

		/**
		* laplacian(W(r,h)) = (-945/(32 pi h^9))(h^2-|r|^2)(-7|r|^2+3h^2)
		*                   = (-945/(32 pi h^9))(h^2-r*r)(3 h^2-7 r*r)
		*/
		static float laplacianW(const glm::vec3& r)
		{
			float res;
			const float r2 = glm::length2(r);
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				float tmp = radius2 - r2;
				float tmp2 = 3 * radius2 - 7 * r2;
				res = m_m * tmp * tmp2;
			}
			else
				res = (float)0.;

			return res;
		}


		static float W_zero()
		{
			return m_W_zero;
		}
	};

	class SpikyKernel
	{
	protected:
		static float m_radius;
		static float m_k;
		static float m_l;
		static float m_W_zero;
	public:
		static float getRadius() { return m_radius; }
		static void setRadius(float val)
		{
			m_radius = val;
			const float radius6 = pow(m_radius, static_cast<float>(6.0));
			const float pi = glm::pi<float>();
			m_k = static_cast<float>(15.0) / (pi * radius6);
			m_l = -static_cast<float>(45.0) / (pi * radius6);
			m_W_zero = W(glm::vec3(0.0f));
		}

	public:

		/**
		* W(r,h) = 15/(pi*h^6) * (h-r)^3
		*/
		static float W(const float r)
		{
			float res = 0.0;
			const float r2 = r * r;
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				const float hr3 = pow(m_radius - r, static_cast<float>(3.0));
				res = m_k * hr3;
			}
			return res;
		}

		static float W(const glm::vec3& r)
		{
			float res = 0.0;
			const float r2 = glm::length2(r);
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				const float hr3 = pow(m_radius - sqrt(r2), static_cast<float>(3.0));
				res = m_k * hr3;
			}
			return res;
		}


		/**
		* grad(W(r,h)) = -r(45/(pi*h^6) * (h-r)^2)
		*/
		static glm::vec3 gradW(const glm::vec3& r)
		{
			glm::vec3 res;
			const float r2 = glm::length2(r);
			const float radius2 = m_radius * m_radius;
			if (r2 <= radius2)
			{
				const float r_l = sqrt(r2);
				const float hr = m_radius - r_l;
				const float hr2 = hr * hr;
				res = m_l * hr2 * r * (static_cast<float>(1.0) / r_l);
			}
			else
				res = glm::vec3(0.0f);

			return res;
		}

		static float W_zero()
		{
			return m_W_zero;
		}
	};

} //engine
