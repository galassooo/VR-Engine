#include "Engine.h"

#include <GL/glew.h>
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
 * @brief Initializes OpenGL buffers (VAO, VBOs, EBO) for this mesh.
 *
 * Extracts separate arrays for positions, normals, and texture coordinates
 * from the vertex list, creates a Vertex Array Object, and allocates
 * GPU buffers for efficient rendering.
 */
void Eng::Mesh::initBuffers() {
    if (buffersInitialized)
        return;

    // Extract separate arrays for positions, normals, and texture coordinates.
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoords;
    positions.reserve(vertices.size() * 3);
    normals.reserve(vertices.size() * 3);
    texCoords.reserve(vertices.size() * 2);

    for (const auto& v : vertices) {
        // Position (vec3)
        positions.push_back(v.getPosition().x);
        positions.push_back(v.getPosition().y);
        positions.push_back(v.getPosition().z);
        // Normal (vec3)
        normals.push_back(v.getNormal().x);
        normals.push_back(v.getNormal().y);
        normals.push_back(v.getNormal().z);
        // Texture coordinates (vec2)
        texCoords.push_back(v.getTexCoords().x);
        texCoords.push_back(v.getTexCoords().y);
    }

    // Generate and bind the VAO.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO for positions.
    glGenBuffers(1, &posVBO);
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(ShaderManager::POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(ShaderManager::POSITION_LOCATION);

    // VBO for normals.
    glGenBuffers(1, &normVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(ShaderManager::NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(ShaderManager::NORMAL_LOCATION);

    // VBO for texture coordinates.
    glGenBuffers(1, &texVBO);
    glBindBuffer(GL_ARRAY_BUFFER, texVBO);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(ShaderManager::TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(ShaderManager::TEX_COORD_LOCATION);

    // EBO for indices.
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Unbind VAO.
    glBindVertexArray(0);

    buffersInitialized = true;
    std::cout << "Mesh buffers initialized. VAO: " << vao << std::endl;
}

/**
 * @brief Renders this mesh using its material and buffers.
 *
 * Binds the material, ensures buffers are initialized, and issues
 * a glDrawElements call. Restores previous shader program if changed.
 */
void Eng::Mesh::render()
{
    if (!material) {
        std::cerr << "ERROR: Material is not set for the mesh: "
            << getName() << " ID: " << getId() << std::endl;
        return;
    }

    auto& sm = ShaderManager::getInstance();

    //materials can have other programs, so save the current one
    auto prevProgram = sm.getCurrentProgram();

    material->render();

    if (!buffersInitialized)
        initBuffers();

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(indices.size()),
        GL_UNSIGNED_INT,
        nullptr);
    glBindVertexArray(0);

    // restore previous program if changed
    if (prevProgram && sm.getCurrentProgram() != prevProgram) {
        sm.loadProgram(prevProgram);
    }

    if (Eng::Base::engIsEnabled(ENG_RENDER_NORMAL))
        renderNormals();
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

/**
 * @brief Sets the center of the mesh's bounding sphere.
 *
 * @param center World-space center of bounding sphere.
 */
void Eng::Mesh::setBoundingSphereCenter(const glm::vec3& center) {
    boundingSphereCenter = center;
}

/**
 * @brief Gets the center of the mesh's bounding sphere.
 *
 * @return World-space center of bounding sphere.
 */
glm::vec3 Eng::Mesh::getBoundingSphereCenter() const {
    return boundingSphereCenter;
}

/**
 * @brief Sets the radius of the mesh's bounding sphere.
 *
 * @param radius Radius value for sphere culling.
 */
void Eng::Mesh::setBoundingSphereRadius(float radius) {
    boundingSphereRadius = radius;
}

/**
 * @brief Gets the radius of the mesh's bounding sphere.
 *
 * @return Radius used for sphere culling.
 */
float Eng::Mesh::getBoundingSphereRadius() const {
    return boundingSphereRadius;
}

/**
 * @brief Sets the axis-aligned bounding box for this mesh.
 *
 * @param min Minimum corner in local space.
 * @param max Maximum corner in local space.
 */
void Eng::Mesh::setBoundingBox(const glm::vec3& min, const glm::vec3& max) {
    boundingBoxMin = min;
    boundingBoxMax = max;
}

/**
 * @brief Gets the minimum corner of the mesh's bounding box.
 *
 * @return Local-space minimum corner.
 */
glm::vec3 Eng::Mesh::getBoundingBoxMin() const {
    return boundingBoxMin;
}

/**
 * @brief Gets the maximum corner of the mesh's bounding box.
 *
 * @return Local-space maximum corner.
 */
glm::vec3 Eng::Mesh::getBoundingBoxMax() const {
    return boundingBoxMax;
}