#pragma once

/**
 * @class Mesh
 * @brief Represents a 3D mesh with vertices, indices, and material data.
 */
class ENG_API Mesh : public Eng::Node {
public:
   Mesh();

   ~Mesh() override;

   void setVertices(const std::vector<Eng::Vertex> &vertices);
   void setIndices(const std::vector<unsigned int> &indices);

   std::vector<Eng::Vertex> &getVertices();
   std::vector<unsigned int> &getIndices();

   void setMaterial(const std::shared_ptr<Eng::Material> &material);
   std::shared_ptr<Eng::Material> getMaterial() const;

   void render() override;

   // method to initialize GPU buffers
   void initBuffers();

private:
   void renderNormals() const;
   ///> Vector of vertex positions.
   std::vector<Eng::Vertex> vertices;
   ///> Vector of indices defining the mesh faces.
   std::vector<unsigned int> indices;
   ///> The material applied to the mesh.
   std::shared_ptr<Eng::Material> material;

   // Hold GPU resource IDs
   unsigned int vao = 0;
   unsigned int posVBO = 0;
   unsigned int normVBO = 0;
   unsigned int texVBO = 0;
   bool buffersInitialized = false;
};
