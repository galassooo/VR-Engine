#include "../engine.h"

/**
 * @brief Tests the functionality of DirectionalLight.
 */
void Eng::testDirectionalLight() {
    glm::vec3 color(1.0f, 1.0f, 1.0f); // White color
    glm::vec3 direction(0.0f, -1.0f, 0.0f); // Downward direction

    // Create DirectionalLight
    Eng::DirectionalLight light(color, direction);

    // Test getDirection
    glm::vec3 retrievedDirection = light.getDirection();
    assert(retrievedDirection == direction);

    std::cout << "DirectionalLight Test Passed!" << std::endl;
}

/**
 * @brief Tests the functionality of PointLight.
 */
void Eng::testPointLight() {
    glm::vec3 color(1.0f, 0.0f, 0.0f); // Red color
    float attenuation = 0.5f; // Example attenuation factor

    // Create PointLight
    Eng::PointLight light(color, attenuation);

    // Test getPosition
    glm::vec3 retrievedPosition = light.getPosition();
    // The position is undefined in this context, assume it to be at the origin (mock test)
    assert(retrievedPosition == glm::vec3(0.0f, 0.0f, 0.0f));

    std::cout << "PointLight Test Passed!" << std::endl;
}

/**
 * @brief Tests the functionality of SpotLight.
 */
void Eng::testSpotLight() {
    glm::vec3 color(0.0f, 1.0f, 0.0f); // Green color
    glm::vec3 direction(0.0f, -1.0f, 0.0f); // Downward direction
    float cutoffAngle = 45.0f;
    float falloff = 0.3f;
    float radius = 10.0f;

    // Create SpotLight
    Eng::SpotLight light(color, direction, cutoffAngle, falloff, radius);

    // Test getDirection
    glm::vec3 retrievedDirection = light.getDirection();
    assert(retrievedDirection == direction);

    // Test getPosition
    glm::vec3 retrievedPosition = light.getPosition();
    // Assume position to be at the origin for the mock test
    assert(retrievedPosition == glm::vec3(0.0f, 0.0f, 0.0f));

    std::cout << "SpotLight Test Passed!" << std::endl;
}