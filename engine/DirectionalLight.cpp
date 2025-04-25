#include "engine.h"
#include <GL/freeglut.h>

/**
 * @brief Constructor for DirectionalLight.
 *
 * @param color The base color of the light (affects ambient, diffuse, and specular).
 * @param direction The direction of the light rays.
 */

Eng::DirectionalLight::DirectionalLight(const glm::vec3 &color, const glm::vec3 &direction) : Light{color},
   direction{glm::normalize(direction)} {
}

/**
 * @brief Renders the directional light in OpenGL.
 *
 * Configures the light's direction and calls the base class to set shared properties.
 *
 * @param index The index of the light (0-7, corresponding to GL_LIGHT0 to GL_LIGHT7).
 */

void Eng::DirectionalLight::configureLight(const glm::mat4 &viewMatrix) {
   // Note that we first negate the light.direction vector.
   // The lighting calculations we used so far expect the light direction to be
   // a direction from the fragment towards the light source, but people generally prefer to
   // specify a directional light as a global direction pointing from the light source.
   // Therefore we have to negate the global light direction vector to switch its direction;
   // it's now a direction vector pointing towards the light source. Also,
   // be sure to normalize the vector since it is unwise to assume the input
   // vector to be a unit vector.

   // REF: https://learnopengl.com/Lighting/Light-casters
   // ALTRA REF: SLIDE LIGHTS DEL CORSO -> IL VETTORE L VA DAL PUNTO ILLUMINATO ALLA LUCEEEEEEE

   //glm::vec3 wDir = -this->direction; //MENOOOOOOOOOOO //normalizzato nel costruttore
   //GLfloat lightDir[] = {direction.x, direction.y, direction.z, 0.0f}; // w=0 for directional
   //glLightfv(lightId, GL_POSITION, lightDir);

   glm::vec3 eDir = glm::mat3(viewMatrix) * direction;
   eDir = glm::normalize(eDir);

   auto& sm = ShaderManager::getInstance();
   sm.setLightDirection(eDir);
   //sm.setLightDirection(direction);
}

/**
 * @brief Gets the current direction of the directional light.
 *
 * @return glm::vec3 The normalized direction of the light rays.
 */

glm::vec3 Eng::DirectionalLight::getDirection() const {
   return direction;
}

glm::mat4 Eng::DirectionalLight::getLightViewMatrix(glm::vec3& center, float range) {
    float halfRange = range * 0.5f;

    if (glm::length(direction) == 0.0f) {
        std::cerr << "ERROR: Direction vector is zero!" << std::endl;
    }

    // Fake light position: moved back in opposite direction to its own
    glm::vec3 lightPos = center - direction * halfRange;

    if (glm::distance(lightPos, center) < 0.0001f) {
        std::cerr << "ERROR: lightPos and center too close or equal!" << std::endl;
    }

    glm::vec3 up = fabs(glm::dot(direction, glm::vec3(0, 1, 0))) > 0.99f ?
        glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);

    glm::mat4 lightView = glm::lookAt(lightPos, center, up);

    return lightView;
}
