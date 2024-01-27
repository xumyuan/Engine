#include "Mesh.h"

namespace engine {
	namespace graphics {

		Mesh::Mesh() {}

		Mesh::Mesh(std::vector<glm::vec3> positions, std::vector<unsigned int> indices)
			: m_Positions(positions), m_Indices(indices) {}

		Mesh::Mesh(std::vector<glm::vec3> positions, std::vector<glm::vec2> uvs, std::vector<unsigned int> indices)
			: m_Positions(positions), m_UVs(uvs), m_Indices(indices) {}

		Mesh::Mesh(std::vector<glm::vec3> positions, std::vector<glm::vec2> uvs, std::vector<glm::vec3> normals, std::vector<unsigned int> indices)
			: m_Positions(positions), m_UVs(uvs), m_Normals(normals), m_Indices(indices) {}

		Mesh::Mesh(std::vector<glm::vec3> positions, std::vector<glm::vec2> uvs, std::vector<glm::vec3> normals, std::vector<glm::vec3> tangents, std::vector<glm::vec3> bitangents, std::vector<unsigned int> indices)
			: m_Positions(positions), m_UVs(uvs), m_Normals(normals), m_Tangents(tangents), m_Bitangents(bitangents) {}


		void Mesh::Draw() const {
			glBindVertexArray(vao);
			if (m_Indices.size() > 0) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				GLenum error = glGetError();
				if (error != GL_NO_ERROR) {
					std::cout << "OpenGL Error: " << error << std::endl;
				}
				glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			else {
				glDrawArrays(GL_TRIANGLES, 0, m_Positions.size());
			}
			glBindVertexArray(0);
		}

		void Mesh::LoadData(bool interleaved) {

			// ��������ʼ������
			{
				unsigned int vertexCount = m_Positions.size();

				if (vertexCount == 0)
					utils::Logger::getInstance().error("logged_files/mesh_creation.txt", "Mesh Creation", "Mesh doesn't contain any vertices");

				if (m_UVs.size() != 0 && m_UVs.size() != vertexCount)
					utils::Logger::getInstance().error("logged_files/mesh_creation.txt", "Mesh Creation", "Mesh UV count doesn't match the vertex count");
				if (m_Normals.size() != 0 && m_Normals.size() != vertexCount)
					utils::Logger::getInstance().error("logged_files/mesh_creation.txt", "Mesh Creation", "Mesh Normal count doesn't match the vertex count");
				if (m_Tangents.size() != 0 && m_Tangents.size() != vertexCount)
					utils::Logger::getInstance().error("logged_files/mesh_creation.txt", "Mesh Creation", "Mesh Tangent count doesn't match the vertex count");
				if (m_Bitangents.size() != 0 && m_Bitangents.size() != vertexCount)
					utils::Logger::getInstance().error("logged_files/mesh_creation.txt", "Mesh Creation", "Mesh Bitangent count doesn't match the vertex count");
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

			// Compute the component count
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

			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glGenBuffers(1, &ebo);

			// �������ݵ�index buffer ��vertex buffer
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
			if (m_Indices.size() > 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), &m_Indices[0], GL_STATIC_DRAW);
			}

			// Setup the format for the VAO
			if (interleaved) {
				size_t stride = bufferComponentCount * sizeof(float);
				size_t offset = 0;

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
				offset += 3 * sizeof(float);
				if (m_Normals.size() > 0) {
					glEnableVertexAttribArray(1);
					glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
					offset += 3 * sizeof(float);
				}
				if (m_UVs.size() > 0) {
					glEnableVertexAttribArray(2);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
					offset += 2 * sizeof(float);
				}
				if (m_Tangents.size() > 0) {
					glEnableVertexAttribArray(3);
					glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
					offset += 3 * sizeof(float);
				}
				if (m_Bitangents.size() > 0) {
					glEnableVertexAttribArray(4);
					glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
					offset += 3 * sizeof(float);
				}
			}
			else {
				size_t offset = 0;

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
				offset += m_Positions.size() * 3 * sizeof(float);
				if (m_Normals.size() > 0) {
					glEnableVertexAttribArray(1);
					glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
					offset += m_Normals.size() * 3 * sizeof(float);
				}
				if (m_UVs.size() > 0) {
					glEnableVertexAttribArray(2);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)offset);
					offset += m_UVs.size() * 2 * sizeof(float);
				}
				if (m_Tangents.size() > 0) {
					glEnableVertexAttribArray(3);
					glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
					offset += m_Tangents.size() * 3 * sizeof(float);
				}
				if (m_Bitangents.size() > 0) {
					glEnableVertexAttribArray(4);
					glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
					offset += m_Bitangents.size() * 3 * sizeof(float);
				}
			}

			glBindVertexArray(0);
		}

	}
}