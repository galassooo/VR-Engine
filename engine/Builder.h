#pragma once

class ENG_API Builder {
public:
	// Return the singleton instance
	static Eng::Builder& getInstance();

	//Disable copy and assignemtn operators
	Builder(const Eng::Builder&) = delete;
	Eng::Builder& operator=(const Eng::Builder&) = delete;

	// Interface methods to accumulate mesh data, used in chaining 
	Builder& addVertex(const Eng::Vertex& vertex);
	Builder& addVertices(const std::vector<Eng::Vertex>& verts);
	Builder& addIndex(unsigned int index);
	Builder& addIndices(const std::vector<unsigned int>& inds);
	Builder& setMaterial(const std::shared_ptr<Eng::Material>& mat);
	Builder& setName(const std::string &name);
	Builder& setLocalMatrix(const glm::mat4 &matrix);

	// Build a Mesh and upload its data to GPU
	std::shared_ptr<Eng::Mesh> build();
private:
	// Private constructor for singleton
	Builder() = default;
	~Builder() = default;

	// Accumulated data.
	std::vector<Eng::Vertex> vertices;
	std::vector<unsigned int> indices;
	std::shared_ptr<Eng::Material> material;
	std::string meshName;
	glm::mat4 localMatrix = glm::mat4(1.0f);
	
	// Free internal builder state so it can be reused
	void free();
};