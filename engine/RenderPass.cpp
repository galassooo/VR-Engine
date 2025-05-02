#include "engine.h"

#include <GL/glew.h>

void Eng::RenderPass::render(Eng::ListElement element) {
    auto& sm = ShaderManager::getInstance();
    // rembember current OpenGL blendig state
    bool blendingEnabled = glIsEnabled(GL_BLEND);
    GLint srcRGB = 0;
    GLint dstRGB = 0;
    if (blendingEnabled) {
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    }

    if (blendingMode) {
        // Set additive blending mode
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        //glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE); // ignores alpha channel in blendind

        // Disable depth writing
        glDepthMask(GL_FALSE);
        // Set depth test function to less or equal to allow overlapping
        glDepthFunc(GL_LEQUAL);
    }
    else {
        // Set default blending mode
        glDisable(GL_BLEND);
        // Enable depth writing
        glDepthMask(GL_TRUE);
        // Set default depth test function
        glDepthFunc(GL_LESS);
    }

    const int size = elements.size();
    int skipped = 0;
    for (int i = lightsCount; i < size; ++i) {
        if (!isAdditive && elements[i]->getLayer() == RenderLayer::Transparent)
            continue;
        // Check if the element is a Mesh and if culling is enabled
        // If sphere culling is enabled and the element is a Mesh, check if it is within the culling sphere
        if (useCulling) {
            if (const auto& mesh = dynamic_cast<Eng::Mesh*>(elements[i]->getNode().get())) {
                // If the mesh is not within the culling sphere, skip rendering
                if (!isWithinCullingSphere(mesh)) {
                    skipped++;
                    continue;
                }
            }
        }

        // Load global light color
        sm.setGlobalLightColor(globalLightColor);

        // Load projection matrix
        sm.setProjectionMatrix(eyeProjectionMatrix);

        glm::mat4 modelMatrix = elements[i]->getWorldCoordinates();

        // Generate modelView matrix
        glm::mat4 modelViewMatrix = eyeViewMatrix * modelMatrix;

        // glLoadMatrixf(glm::value_ptr(modelViewMatrix));    unsupported 4.4

        // Send 4x4 modelview matrix
        sm.setModelViewMatrix(modelViewMatrix);

        // Send 3x3 inverse-transpose for normals
        glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelViewMatrix));
        sm.setNormalMatrix(normalMat);

        // Send lightSpaceModel matrix
        glm::mat4 modelLightMatrix = lightSpaceMatrix * modelMatrix;
        sm.setLightSpaceMatrix(modelLightMatrix);

        // Send eye front vector: this is the camera front vector in world coordinates
        // which corresponds to the third column of the view matrix
        glm::vec3 eyeFront = -glm::vec3(glm::transpose(glm::mat3(eyeViewMatrix))[2]);
        sm.setEyeFront(eyeFront);

        elements[i]->getNode()->render();
}