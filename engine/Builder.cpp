#include "engine.h"
#include <GL/glew.h>

/**
 * @brief Retrieves the singleton instance of the Builder.
 *
 * Ensures only one Builder exists during application runtime.
 *
 * @return Reference to the unique Builder instance.
 */
Eng::Builder& Eng::Builder::getInstance() {
	static Builder instance;
	return instance;
}

/**
 * @brief Adds a single vertex to the builder.
 *
 * @param vertex The vertex data to append.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::addVertex(const Vertex& vertex) {
	vertices.push_back(vertex);
	return *this;
}

/**
 * @brief Adds multiple vertices to the builder.
 *
 * Appends a list of vertices to the internal vertex buffer.
 *
 * @param verts Vector of vertices to append.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::addVertices(const std::vector<Vertex>& verts) {
	vertices.insert(vertices.end(), verts.begin(), verts.end());
	return *this;
}

/**
 * @brief Adds a single index to the builder.
 *
 * @param index The index value to append.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::addIndex(unsigned int index) {
	indices.push_back(index);
	return *this;
}

/**
 * @brief Adds multiple indices to the builder.
 *
 * Appends a list of index values to the internal index buffer.
 *
 * @param inds Vector of indices to append.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::addIndices(const std::vector<unsigned int>& inds) {
	indices.insert(indices.end(), inds.begin(), inds.end());
	return *this;
}

/**
 * @brief Sets the material for the mesh being built.
 *
 * @param mat Shared pointer to the Material to assign.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::setMaterial(const std::shared_ptr<Eng::Material>& mat) {
	material = mat;
	return *this;
}

/**
 * @brief Assigns a name to the mesh being built.
 *
 * @param name The name to set for the mesh.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::setName(const std::string& name) {
	meshName = name;
	return *this;
}

/**
 * @brief Sets the local transformation matrix for the mesh.
 *
 * @param matrix The 4x4 transformation matrix.
 * @return Reference to this Builder for chaining.
 */
Eng::Builder& Eng::Builder::setLocalMatrix(const glm::mat4& matrix) {
	localMatrix = matrix;
	return *this;
}

/**
 * @brief Builds the Mesh from accumulated data and uploads to GPU.
 *
 * Creates a new Mesh instance, assigns vertices, indices, material,
 * name, and local matrix, initializes GPU buffers, then resets the Builder's state.
 *
 * @return Shared pointer to the newly created Mesh.
 */
std::shared_ptr<Eng::Mesh> Eng::Builder::build() {
	// Create a new Mesh instance and assign accumulated data.
	auto mesh = std::make_shared<Eng::Mesh>();
	mesh->setVertices(vertices);
	mesh->setIndices(indices);
	mesh->setMaterial(material);
	mesh->setName(std::move(meshName));
	mesh->setLocalMatrix(localMatrix);

	// Initialize GPU buffers.
	mesh->initBuffers();

	// Clear the Builder's internal state for reuse.
	free();

	return mesh;
}

/**
 * @brief Resets the Builder's internal state for reuse.
 *
 * Clears stored vertices, indices, material, name, and resets the local matrix to identity.
 */
void Eng::Builder::free() {
	vertices.clear();
	indices.clear();
	material.reset();
	meshName.clear();
	localMatrix = glm::mat4(1.0f);
}