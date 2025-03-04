#include "engine.h"
#include <GL/glew.h>

// Singleton instance
Eng::Builder& Eng::Builder::getInstance() {
	static Builder instance;
	return instance;
}

Eng::Builder& Eng::Builder::addVertex(const Vertex& vertex) {
	vertices.push_back(vertex);
	return *this;
}

Eng::Builder& Eng::Builder::addVertices(const std::vector<Vertex>& verts) {
	vertices.insert(vertices.end(), verts.begin(), verts.end());
	return *this;
}

Eng::Builder& Eng::Builder::addIndex(unsigned int index) {
	indices.push_back(index);
	return *this;
}

Eng::Builder& Eng::Builder::addIndices(const std::vector<unsigned int>& inds) {
	indices.insert(indices.end(), inds.begin(), inds.end());
	return *this;
}

Eng::Builder& Eng::Builder::setMaterial(const std::shared_ptr<Eng::Material>& mat) {
	material = mat;
	return *this;
}

std::shared_ptr<Eng::Mesh> Eng::Builder::build() {
	// Create a new Mesh instance and assign accumulated data.
	auto mesh = std::make_shared<Eng::Mesh>();
	mesh->setVertices(vertices);
	mesh->setIndices(indices);
	mesh->setMaterial(material);

	// Initialize GPU buffers
	mesh->initBuffers();

	// Clear the Builder's internal state for reuse
	free();

	return mesh;
}

void Eng::Builder::free() {
	vertices.clear();
	indices.clear();
	material.reset();
}