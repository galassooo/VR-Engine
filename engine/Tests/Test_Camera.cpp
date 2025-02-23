#include "../engine.h"

/**
 * @brief Tests the functionality of OrthographicCamera.
 */
void Eng::testOrthographicCamera() {
    Eng::OrthographicCamera cam(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);

    // Retrieve projection matrix
    glm::mat4 projMatrix = cam.getProjectionMatrix();

    // Expected projection matrix
    glm::mat4 expectedProjMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);

    // Validate
    assert(projMatrix == expectedProjMatrix);
    std::cout << "OrthographicCamera Test Passed!" << std::endl;
}

/**
 * @brief Tests the functionality of PerspectiveCamera.
 */
void Eng::testPerspectiveCamera() {
    Eng::PerspectiveCamera cam(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);

    // Set aspect ratio
    cam.setAspect(16.0f / 9.0f);

    // Retrieve projection matrix
    glm::mat4 projMatrix = cam.getProjectionMatrix();

    // Expected projection matrix
    glm::mat4 expectedProjMatrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    // Validate
    assert(projMatrix == expectedProjMatrix);
    std::cout << "PerspectiveCamera Test Passed!" << std::endl;
}