#include "engine.h"

#include <GL/freeglut.h>

#ifdef _WINDOWS

#else
#include <execinfo.h>
#endif


/**
 * @brief Default constructor for the Mesh class.
 *
 * Initializes an empty mesh with no vertices, indices, or material.
 */
Eng::Mesh::Mesh() = default;

/**
 * @brief Default destructor for the Mesh class.
 */
Eng::Mesh::~Mesh() = default;

/**
 * @brief Sets the vertices of the mesh.
 *
 * Assigns a vector of vertex data to the mesh. Each vertex includes position,
 * normal, and texture coordinate information.
 *
 * @param verts A vector of Vertex objects representing the geometry of the mesh.
 */
void Eng::Mesh::setVertices(const std::vector<Vertex> &verts) {
   vertices = verts;
}

/**
 * @brief Sets the indices for defining the mesh's faces.
 *
 * The indices define the order in which vertices are combined into triangles
 * for rendering. Each set of three indices forms one triangle.
 *
 * @param inds A vector of unsigned integers representing the triangle vertex order.
 */
void Eng::Mesh::setIndices(const std::vector<unsigned int> &inds) {
   indices = inds;
}

/**
 * @brief Sets the material for the mesh.
 *
 * The material defines the appearance of the mesh, including its color, shininess,
 * and optional texture.
 *
 * @param mat A shared pointer to the Material object to be applied to the mesh.
 */
void Eng::Mesh::setMaterial(const std::shared_ptr<Eng::Material> &mat) {
   material = mat;
}

/**
 * @brief Retrieves the material associated with the mesh.
 *
 * @return std::shared_ptr<Eng::Material> A shared pointer to the current material.
 */
std::shared_ptr<Eng::Material> Eng::Mesh::getMaterial() const {
   return material;
}

/**
 * @brief Renders the mesh using the provided model-view matrix.
 *
 * This method applies the material settings, loads the transformations, and
 * draws the mesh using OpenGL. It iterates through the indices to form triangles,
 * reversing the order of vertices for each triangle.
 *
 * @param index The index of the mesh in the render list.
 */
void Eng::Mesh::render() {
   if (!material) {
      std::cerr << "ERROR: Material is not set for the mesh: " << getName() << " ID: " << getId() << std::endl;
      return;
   }
   // Apply material settings
   material->render();

   if  (Eng::Base::engIsEnabled(ENG_RENDER_NORMAL)) {
      renderNormals();
   }
   // Load the transformations matrix into OpenGL
   glBegin(GL_TRIANGLES);
   for (size_t i = 0; i < indices.size(); i += 3) {
      // Reverse the order of vertices for each triangle
      for (int j = 0; j < 3; ++j) {
         const Vertex &vertex = vertices[indices[i + j]];
         // Set normal
         glNormal3fv(glm::value_ptr(vertex.getNormal()));
         // Set texture coordinates
         glTexCoord2fv(glm::value_ptr(vertex.getTexCoords()));
         // Set vertex position
         glVertex3fv(glm::value_ptr(vertex.getPosition()));
      }
   }
   glEnd();
}

/**
 * @brief Retrieves the vertices of the mesh.
 *
 * Provides access to the list of vertices that define the geometry of the mesh.
 *
 * @return std::vector<Eng::Vertex>& A reference to the vector of vertices.
 */
std::vector<Eng::Vertex> &Eng::Mesh::getVertices() {
   return vertices;
}

/**
 * @brief Retrieves the indices of the mesh.
 *
 * The indices define how the vertices are connected to form triangles in the mesh.
 * These are essential for rendering the mesh efficiently.
 *
 * @return std::vector<unsigned int>& A reference to the vector of indices.
 */
std::vector<unsigned int> &Eng::Mesh::getIndices() {
   return indices;
}

/**
* @brief Renders normal vectors for each vertex of the mesh.
*
* Draws yellow lines representing vertex normals. Each line starts at the vertex
* position and extends in the normal direction with a length of 0.5 units.
* Temporarily disables lighting during rendering.
*/
void Eng::Mesh::renderNormals() const {
   glDisable(GL_LIGHTING);
   glColor3f(1.0f, 1.0f, 0.0f);
   glBegin(GL_LINES);
   for (const auto& vertex : vertices) {
      glm::vec3 pos = vertex.getPosition();
      glm::vec3 normal = vertex.getNormal();
      glm::vec3 end = pos + normal * 0.5f;

      glVertex3fv(glm::value_ptr(pos));
      glVertex3fv(glm::value_ptr(end));
   }
   glEnd();
   glEnable(GL_LIGHTING);
}
