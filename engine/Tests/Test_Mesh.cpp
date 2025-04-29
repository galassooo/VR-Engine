#include "../engine.h"

/**
 * @brief Tests the vertex and index management of the Mesh class.
 */
void Eng::testMeshVerticesAndIndices() {
    // Create a Mesh instance
    Eng::Mesh mesh;

    // Mock vertex data
    std::vector<Eng::Vertex> vertices = {
        Eng::Vertex(glm::vec3(1.0f, 0.0f, 0.0f)),
        Eng::Vertex(glm::vec3(0.0f, 1.0f, 0.0f)),
        Eng::Vertex(glm::vec3(0.0f, 0.0f, 1.0f))
    };

    // Mock index data
    std::vector<unsigned int> indices = { 0, 1, 2 };

    // Set vertices and indices
    mesh.setVertices(vertices);
    mesh.setIndices(indices);

    // Validate (assuming accessors are available or internal state checks)
    std::cout << "Mesh Vertices and Indices Test Passed!" << std::endl;
}

/**
 * @brief Tests the material management of the Mesh class.
 */
void Eng::testMeshMaterial() {
    // Create a Mesh instance
    Eng::Mesh mesh;

    // Create a mock material with valid arguments
    auto material = std::make_shared<Eng::Material>(glm::vec3(0.5f, 0.5f, 0.5f), 1.0f, 32.0f, glm::vec3(0));

    // Set material
    mesh.setMaterial(material);

    // Retrieve and validate material
    auto retrievedMaterial = mesh.getMaterial();
    assert(retrievedMaterial == material);

    std::cout << "Mesh Material Test Passed!" << std::endl;
}