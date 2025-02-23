//
// Created by Martina ♛ on 08/12/24.
//

#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>


class ENG_API Vertex final {
public:
   explicit Vertex(const glm::vec3 &position = glm::vec3(0.0f),
                   const glm::vec3 &normal = glm::vec3(0.0f),
                   const glm::vec2 &texCoords = glm::vec2(0.0f));

   const glm::vec3 &getPosition() const;
   const glm::vec3 &getNormal() const;
   const glm::vec2 &getTexCoords() const;

   void setPosition(const glm::vec3 &position);
   void setNormal(const glm::vec3 &normal);
   void setTexCoords(const glm::vec2 &texCoords);

private:
   ///> Texture height.
   glm::vec3 position;
   ///> The normal vector of the vertex for lighting calculations
   glm::vec3 normal;
   ///> The UV texture coordinates for mapping textures
   glm::vec2 texCoords;
};
