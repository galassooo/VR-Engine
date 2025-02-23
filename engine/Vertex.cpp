//
// Created by Martina ♛ on 08/12/24.
//

#include "engine.h"

/**
 * @brief Constructor for Vertex class.
 *
 * @param position The 3D position of the vertex.
 * @param normal The normal vector of the vertex.
 * @param texCoords The texture coordinates of the vertex.
 */
Eng::Vertex::Vertex(const glm::vec3 &position, const glm::vec3 &normal, const glm::vec2 &texCoords)
   : position(position), normal(normal), texCoords(texCoords) {
}

/**
 * @brief Gets the position of the vertex.
 * @return const glm::vec3& Reference to the vertex position.
 */
const glm::vec3 &Eng::Vertex::getPosition() const {
   return position;
}

/**
 * @brief Gets the normal vector of the vertex.
 * @return const glm::vec3& Reference to the vertex normal.
 */
const glm::vec3 &Eng::Vertex::getNormal() const {
   return normal;
}

/**
 * @brief Gets the texture coordinates of the vertex.
 * @return const glm::vec2& Reference to the texture coordinates.
 */
const glm::vec2 &Eng::Vertex::getTexCoords() const {
   return texCoords;
}

/**
 * @brief Sets the position of the vertex.
 * @param position The new position vector to set.
 */
void Eng::Vertex::setPosition(const glm::vec3 &position) {
   this->position = position;
}

/**
 * @brief Sets the normal vector of the vertex.
 * @param normal The new normal vector to set.
 */
void Eng::Vertex::setNormal(const glm::vec3 &normal) {
   this->normal = normal;
}

/**
 * @brief Sets the texture coordinates of the vertex.
 * @param texCoords The new texture coordinates to set.
 */
void Eng::Vertex::setTexCoords(const glm::vec2 &texCoords) {
   this->texCoords = texCoords;
}
