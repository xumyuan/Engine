#include "pch.h"
#include "Mesh.h"
#include "utils/DebugEvent.h"

namespace engine {

	Mesh::Mesh() : m_Device(getRHIDevice()) {}

	Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<unsigned int>& indices)
		: m_Device(getRHIDevice()), m_Positions(positions), m_Indices(indices) {
	}

	Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<unsigned int>& indices)
		: m_Device(getRHIDevice()), m_Positions(positions), m_UVs(uvs), m_Indices(indices) {
	}

	Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<unsigned int>& indices)
		: m_Device(getRHIDevice()), m_Positions(positions), m_UVs(uvs), m_Normals(normals), m_Indices(indices) {
	}

	Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& tangents, std::vector<glm::vec3>& bitangents, std::vector<unsigned int>& indices)
		: m_Device(getRHIDevice()), m_Positions(positions), m_UVs(uvs), m_Normals(normals), m_Tangents(tangents), m_Bitangents(bitangents), m_Indices(indices) {
	}

	Mesh::~Mesh() {
		if (m_Device) {
			if (static_cast<bool>(m_RenderPrimitive))
				m_Device->destroyRenderPrimitive(m_RenderPrimitive);
			if (static_cast<bool>(m_VertexBuffer))
				m_Device->destroyBuffer(m_VertexBuffer);
			if (static_cast<bool>(m_IndexBuffer))
				m_Device->destroyBuffer(m_IndexBuffer);
		}
	}

	Mesh::Mesh(Mesh&& other) noexcept
		: m_Device(other.m_Device),
		  m_VertexBuffer(other.m_VertexBuffer),
		  m_IndexBuffer(other.m_IndexBuffer),
		  m_RenderPrimitive(other.m_RenderPrimitive),
		  m_Material(std::move(other.m_Material)),
		  m_Positions(std::move(other.m_Positions)),
		  m_UVs(std::move(other.m_UVs)),
		  m_Normals(std::move(other.m_Normals)),
		  m_Tangents(std::move(other.m_Tangents)),
		  m_Bitangents(std::move(other.m_Bitangents)),
		  m_Indices(std::move(other.m_Indices))
	{
		// 清除 other 的句柄，防止析构时双重销毁
		other.m_VertexBuffer.clear();
		other.m_IndexBuffer.clear();
		other.m_RenderPrimitive.clear();
		other.m_Device = nullptr;
	}

	Mesh& Mesh::operator=(Mesh&& other) noexcept {
		if (this != &other) {
			// 先销毁自己的资源
			if (m_Device) {
				if (static_cast<bool>(m_RenderPrimitive))
					m_Device->destroyRenderPrimitive(m_RenderPrimitive);
				if (static_cast<bool>(m_VertexBuffer))
					m_Device->destroyBuffer(m_VertexBuffer);
				if (static_cast<bool>(m_IndexBuffer))
					m_Device->destroyBuffer(m_IndexBuffer);
			}

			// 转移资源
			m_Device = other.m_Device;
			m_VertexBuffer = other.m_VertexBuffer;
			m_IndexBuffer = other.m_IndexBuffer;
			m_RenderPrimitive = other.m_RenderPrimitive;
			m_Material = std::move(other.m_Material);
			m_Positions = std::move(other.m_Positions);
			m_UVs = std::move(other.m_UVs);
			m_Normals = std::move(other.m_Normals);
			m_Tangents = std::move(other.m_Tangents);
			m_Bitangents = std::move(other.m_Bitangents);
			m_Indices = std::move(other.m_Indices);

			// 清除 other
			other.m_VertexBuffer.clear();
			other.m_IndexBuffer.clear();
			other.m_RenderPrimitive.clear();
			other.m_Device = nullptr;
		}
		return *this;
	}

	void Mesh::Draw() const {
		if (!m_Device || !static_cast<bool>(m_RenderPrimitive)) return;

		m_Device->bindRenderPrimitive(m_RenderPrimitive);
		if (m_Indices.size() > 0) {
			m_Device->draw(static_cast<uint32_t>(m_Indices.size()), 0);
		} else {
			m_Device->drawArrays(rhi::PrimitiveType::Triangles,
				static_cast<uint32_t>(m_Positions.size()));
		}
	}

	void Mesh::LoadData(bool interleaved) {
		if (!m_Device) {
			m_Device = getRHIDevice();
		}
		if (!m_Device) {
			spdlog::error("[Mesh] RHI device not available");
			return;
		}

		// 检查网格初始化错误
		{
			unsigned int vertexCount = m_Positions.size();

			if (vertexCount == 0)
				spdlog::error("[Mesh Creation] Mesh doesn't contain any vertices");

			if (m_UVs.size() != 0 && m_UVs.size() != vertexCount)
				spdlog::error("Mesh UV count doesn't match the vertex count");
			if (m_Normals.size() != 0 && m_Normals.size() != vertexCount)
				spdlog::error("Mesh Normal count doesn't match the vertex count");
			if (m_Tangents.size() != 0 && m_Tangents.size() != vertexCount)
				spdlog::error("Mesh Tangent count doesn't match the vertex count");
			if (m_Bitangents.size() != 0 && m_Bitangents.size() != vertexCount)
				spdlog::error("Mesh Bitangent count doesn't match the vertex count");
		}

		// Preprocess the mesh data in the format that was specified
		std::vector<float> data;
		if (interleaved) {
			for (unsigned int i = 0; i < m_Positions.size(); i++) {
				data.push_back(m_Positions[i].x);
				data.push_back(m_Positions[i].y);
				data.push_back(m_Positions[i].z);

				if (m_Normals.size() > 0) {
					data.push_back(m_Normals[i].x);
					data.push_back(m_Normals[i].y);
					data.push_back(m_Normals[i].z);
				}
				if (m_UVs.size() > 0) {
					data.push_back(m_UVs[i].x);
					data.push_back(m_UVs[i].y);
				}
				if (m_Tangents.size() > 0) {
					data.push_back(m_Tangents[i].x);
					data.push_back(m_Tangents[i].y);
					data.push_back(m_Tangents[i].z);
				}
				if (m_Bitangents.size() > 0) {
					data.push_back(m_Bitangents[i].x);
					data.push_back(m_Bitangents[i].y);
					data.push_back(m_Bitangents[i].z);
				}
			}
		}
		else {
			for (unsigned int i = 0; i < m_Positions.size(); i++) {
				data.push_back(m_Positions[i].x);
				data.push_back(m_Positions[i].y);
				data.push_back(m_Positions[i].z);
			}

			for (unsigned int i = 0; i < m_Normals.size(); i++) {
				data.push_back(m_Normals[i].x);
				data.push_back(m_Normals[i].y);
				data.push_back(m_Normals[i].z);
			}

			for (unsigned int i = 0; i < m_UVs.size(); i++) {
				data.push_back(m_UVs[i].x);
				data.push_back(m_UVs[i].y);
			}

			for (unsigned int i = 0; i < m_Tangents.size(); i++) {
				data.push_back(m_Tangents[i].x);
				data.push_back(m_Tangents[i].y);
				data.push_back(m_Tangents[i].z);
			}

			for (unsigned int i = 0; i < m_Bitangents.size(); i++) {
				data.push_back(m_Bitangents[i].x);
				data.push_back(m_Bitangents[i].y);
				data.push_back(m_Bitangents[i].z);
			}
		}

		// Compute the component count & stride
		unsigned int bufferComponentCount = 0;
		if (m_Positions.size() > 0)
			bufferComponentCount += 3;
		if (m_Normals.size() > 0)
			bufferComponentCount += 3;
		if (m_UVs.size() > 0)
			bufferComponentCount += 2;
		if (m_Tangents.size() > 0)
			bufferComponentCount += 3;
		if (m_Bitangents.size() > 0)
			bufferComponentCount += 3;

		// ── 创建 Vertex Buffer ──
		rhi::BufferDesc vbDesc;
		vbDesc.usage = rhi::BufferUsage::Vertex;
		vbDesc.size = static_cast<uint32_t>(data.size() * sizeof(float));
		m_VertexBuffer = m_Device->createBuffer(vbDesc);

		rhi::BufferDataDesc vbData;
		vbData.data = data.data();
		vbData.size = vbDesc.size;
		vbData.offset = 0;
		m_Device->updateBuffer(m_VertexBuffer, vbData);

		// ── 创建 Index Buffer ──
		if (m_Indices.size() > 0) {
			rhi::BufferDesc ibDesc;
			ibDesc.usage = rhi::BufferUsage::Index;
			ibDesc.size = static_cast<uint32_t>(m_Indices.size() * sizeof(unsigned int));
			m_IndexBuffer = m_Device->createBuffer(ibDesc);

			rhi::BufferDataDesc ibData;
			ibData.data = m_Indices.data();
			ibData.size = ibDesc.size;
			ibData.offset = 0;
			m_Device->updateBuffer(m_IndexBuffer, ibData);
		}

		// ── 构建 VertexLayout ──
		rhi::VertexLayout layout = {};
		uint8_t attrIndex = 0;
		uint32_t offset = 0;

		if (interleaved) {
			uint32_t stride = bufferComponentCount * sizeof(float);

			// position (location 0)
			layout.attributes[attrIndex].bufferIndex = 0;
			layout.attributes[attrIndex].offset = offset;
			layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
			offset += 3 * sizeof(float);
			attrIndex++;

			// normal (location 1)
			if (m_Normals.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = offset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
				offset += 3 * sizeof(float);
				attrIndex++;
			}

			// uv (location 2)
			if (m_UVs.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = offset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float2;
				offset += 2 * sizeof(float);
				attrIndex++;
			}

			// tangent (location 3)
			if (m_Tangents.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = offset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
				offset += 3 * sizeof(float);
				attrIndex++;
			}

			// bitangent (location 4)
			if (m_Bitangents.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = offset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
				offset += 3 * sizeof(float);
				attrIndex++;
			}

			layout.strides[0] = stride;
		}
		else {
			uint32_t bufferOffset = 0;

			// position (location 0)
			layout.attributes[attrIndex].bufferIndex = 0;
			layout.attributes[attrIndex].offset = bufferOffset;
			layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
			bufferOffset += static_cast<uint32_t>(m_Positions.size() * 3 * sizeof(float));
			attrIndex++;

			// normal (location 1)
			if (m_Normals.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = bufferOffset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
				bufferOffset += static_cast<uint32_t>(m_Normals.size() * 3 * sizeof(float));
				attrIndex++;
			}

			// uv (location 2)
			if (m_UVs.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = bufferOffset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float2;
				bufferOffset += static_cast<uint32_t>(m_UVs.size() * 2 * sizeof(float));
				attrIndex++;
			}

			// tangent (location 3)
			if (m_Tangents.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = bufferOffset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
				bufferOffset += static_cast<uint32_t>(m_Tangents.size() * 3 * sizeof(float));
				attrIndex++;
			}

			// bitangent (location 4)
			if (m_Bitangents.size() > 0) {
				layout.attributes[attrIndex].bufferIndex = 0;
				layout.attributes[attrIndex].offset = bufferOffset;
				layout.attributes[attrIndex].type = rhi::VertexAttribType::Float3;
				attrIndex++;
			}

			layout.strides[0] = 0; // tightly packed, stride=0 means use component size
		}

		layout.attributeCount = attrIndex;
		layout.bufferCount = 1;

		// ── 创建 RenderPrimitive (VAO) ──
		rhi::BufferHandle vbs[] = { m_VertexBuffer };
		m_RenderPrimitive = m_Device->createRenderPrimitive(
			layout, vbs, 1, m_IndexBuffer, rhi::IndexType::UInt32);
	}

}
