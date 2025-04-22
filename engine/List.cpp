#include "engine.h"

// GLEW
#include <GL/glew.h>

#include <GL/freeglut.h>

/**
 * @brief Default constructor for the List class.
 *
 * Initializes the render list and assigns a default name.
 */
Eng::List::List() : Object() {
   name = "RenderList";
}

/**
 * @brief Adds a node to the render list.
 *
 * Nodes are categorized based on their type:
 * - Light nodes are added to the front of the list.
 * - Other nodes are added to the back of the list.
 *
 * @param node A shared pointer to the node being added.
 * @param finalMatrix The transformation matrix in world space for the node.
 */
void Eng::List::addNode(const std::shared_ptr<Eng::Node> &node, const glm::mat4 &finalMatrix) {
   // Create the ListElement and then sort it
   auto element = std::make_shared<Eng::ListElement>(node, finalMatrix);

   // Keep lights count to use it as an index later
   if (element->getLayer() == RenderLayer::Lights)
       lightsCount++;

   const auto it = std::ranges::find_if(elements, [&element](const auto &e) {
      return element->getLayer() < e->getLayer();
   });

   elements.insert(it, element);
}

/**
 * @brief Clears the render list.
 *
 * Removes all nodes from the list, resetting it for the next frame.
 */
void Eng::List::clear() {
   elements.clear();
   lightsCount = 0;
}

/**
 * @brief Renders all nodes in the render list.
 *
 * This method iterates through all nodes, computes their model-view matrices,
 * and invokes their render methods.
 *
 */
void Eng::List:: render() {
   const int size = elements.size();

   // Virtual Environment
   // Here we do the sphere culling for the virtual environment as it is simpler to work in eye coordinates rather than world coordinates
   // The previous implementation was on the engine traverseAndAdd but it was wrong as we were mixing eye coordinates with world coordinates

   // Determine if we are in stereoscopic mode
   bool stereo = Eng::Base::engIsEnabled(ENG_STEREO_RENDERING);


   // Set up the near and far clip based on the engine's mode
   float nearClip, farClip;
   if (stereo) {
       nearClip = STEREO_NEAR_CLIP;
       farClip = STEREO_FAR_CLIP;
   }
   else
   {
       // Retrieve the active camera values
       auto cam = Eng::Base::getInstance().getActiveCamera();
       if (cam) {
           if (const Eng::PerspectiveCamera* persp = dynamic_cast<const Eng::PerspectiveCamera*>(cam.get())) {
               nearClip = persp->getNearClip();
               farClip = persp->getFarClip();
           }
           else {
               nearClip = 0.01f;
               farClip = 1000.0f;
           }
       }
       else {
           nearClip = 0.01f;
           farClip = 1000.0f;
       }
   }

   // Compute the mid-distance and culling sphere radius
   float midDistance = (nearClip + farClip) * 0.5f;
   float cullingRadius = (farClip - nearClip) * 0.5f;

   // In eye space, the camera (or head) is at (0,0,0)
   glm::vec3 cullingCenter(0.0f, 0.0f, -midDistance);

   for (int i = 0; i < size; ++i) {
      // Generate modelView matrix
      glm::mat4 modelViewMatrix = viewMatrix * elements[i]->getWorldCoordinates();

      // glLoadMatrixf(glm::value_ptr(modelViewMatrix));    unsupported 4.4

      // If this element's node is a Mesh, perfrom sphere culling
      Eng::Mesh* mesh = dynamic_cast<Eng::Mesh*>(elements[i]->getNode().get());
      if (mesh) {
          // The mesh stores its bounding sphere (in local space).
          glm::vec3 localCenter = mesh->getBoundingSphereCenter();
          float localRadius = mesh->getBoundingSphereRadius();

          // Transform the bounding sphere center to eye space.
          glm::vec3 eyeCenter = glm::vec3(modelViewMatrix * glm::vec4(localCenter, 1.0f));

          // For non-uniform scaling, extract an approximate uniform scale:
          float scale = glm::length(glm::vec3(modelViewMatrix[0]));
          float effectiveRadius = localRadius * scale;

          // Compute the squared distance from the transformed center to the culling sphere center.
          glm::vec3 diff = eyeCenter - cullingCenter;
          float distSq = glm::dot(diff, diff);
          float sumRadii = effectiveRadius + cullingRadius;

          // If the object's sphere is completely outside, skip rendering it.
          if (distSq > (sumRadii * sumRadii))
              continue;
      }

      // Send 4x4 modelview matrix
      ShaderManager::getInstance().setModelViewMatrix(modelViewMatrix);

      // Send 3x3 inverse-transpose for normals
      glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelViewMatrix));
      ShaderManager::getInstance().setNormalMatrix(normalMat);

      elements[i]->getNode()->render();
   }
}

/**
 * @brief Renders the elements in the list.
 *
 * This method is called during the rendering process to draw all elements
 * in the list. It handles both additive and non-additive rendering.
 *
 * @param isAdditive A boolean indicating whether to use additive rendering.
 */
void Eng::List::renderPass(bool isAdditive) {
    auto& sm = ShaderManager::getInstance();
    // rembember current OpenGL blendig state
    bool blendingEnabled = glIsEnabled(GL_BLEND);
    //std::cout << "(Render Pass) Blending: " << (blendingEnabled ? "Enabled" : "Disabled") << std::endl;
    GLint srcRGB = 0;
    GLint dstRGB = 0;
    if (blendingEnabled) {
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
    }

    if (isAdditive) {
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
    for (int i = lightsCount; i < size; ++i) {
        // Load projection matrix
        sm.setProjectionMatrix(eyeProjectionMatrix);

        glm::mat4 modelMatrix = elements[i]->getWorldCoordinates();

        // Generate modelView matrix
        glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;

        // glLoadMatrixf(glm::value_ptr(modelViewMatrix));    unsupported 4.4

        // Send 4x4 modelview matrix
        sm.setModelViewMatrix(modelViewMatrix);

        // Send 3x3 inverse-transpose for normals
        glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelViewMatrix));
        sm.setNormalMatrix(normalMat);

        // Send lightSpaceModel matrix
        glm::mat4 modelLightMatrix = lightSpaceMatrix * modelMatrix;
        sm.setLightSpaceMatrix(modelLightMatrix);

        elements[i]->getNode()->render();
    }

    // Reset previous OpenGL blending state
    if (blendingEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(srcRGB, dstRGB);
    }
    else {
        glDisable(GL_BLEND);
    }
}

void Eng::List::shadowPass(std::shared_ptr <Eng::DirectionalLight>& light) {
	auto& sm = ShaderManager::getInstance();
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

	sm.loadProgram(shadowMapProgram);

    float range = 30.0f; //STEREO_FAR_CLIP - STEREO_NEAR_CLIP;
    float halfrange = range * 0.5f;

    // Inverse of camera view matrix
    glm::mat4 invView = glm::inverse(viewMatrix);

    // The center of the view is the origin shifted of -halfrange on z axes on current view coordinates (0, 0, -halfrange)
    // So we only need to convert it back to world space
    glm::vec4 camCenter = invView * glm::vec4(0.0f, 0.0f, -halfrange, 1.0f);
    glm::vec3 lightCenter = glm::vec3(camCenter);

    glm::mat4 lightView = light->getLightViewMatrix(lightCenter, range);

    // calculate and set the lightSpaceMatric for shadow projection
    lightSpaceMatrix = lightProjectionMatrix * lightView;

    // Activate and clean the shadow map FBO
    shadowMapFbo->render();
    glViewport(0, 0, shadowMapFbo->getSizeX(), shadowMapFbo->getSizeY());

    // Enable depth writing
    glDepthMask(GL_TRUE);

    // Clear depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // Run the render pass (no additive)
    renderPass(false);

    // Reset to standard buffer
    Fbo::disable();
    glViewport(0, 0, windowWidth, windowHeight);
}

/**
 * @brief Retrieves all elements in the list.
 * @return A vector of shared pointers to the list elements in the list.
 */
std::vector<std::shared_ptr<Eng::ListElement> > Eng::List::getElements() const {
   return elements;
}

/**
* @brief Set new view matrix in the render list.
* @param glm::mat4 View matrix.
*/
void Eng::List::setViewMatrix(glm::mat4& viewMatrix) {
    this->viewMatrix = viewMatrix;
}

/**
* @brief Set new eye projection matrix in the render list.
* @param glm::mat4 Eye projection matrix.
*/
void Eng::List::setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix) {
	this->eyeProjectionMatrix = eyeProjectionMatrix;
}

/**
 * @brief Sets up the shadow map for rendering.
 *
 * This method creates a framebuffer object (FBO) and a depth texture for shadow mapping.
 * It also computes the ortho projection matrix for rendering shadows.
 *
 * @param width The width of the shadow map texture.
 * @param height The height of the shadow map texture.
 * @param range The range of the light source for shadow mapping.
 * @return true if the setup is successful, false otherwise.
 */
bool Eng::List::setupShadowMap(int width, int height, float range) {
    std::cout << "DEBUG: Setting up shadow map, requested size: "
        << width << "x" << height << ", range: " << range << std::endl;

    if (width <= 0 || height <= 0 || range <= 0.0f) {
        width = 1024;
        height = 1024;
        range = 10.0f;
        std::cout << "DEBUG: Using default shadow map size and range: "
            << width << "x" << height << ", range: " << range << std::endl;
    }

    // Elimina texture e FBO precedenti
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }

    shadowMapFbo = std::make_shared<Fbo>();
    shadowMapFbo->setDepthOnly(true);

    // Crea texture di profondità
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    std::cout << "DEBUG: Shadow depth texture created with ID: " << shadowMapTexture << std::endl;

    // Bind texture all'FBO
    shadowMapFbo->bindTexture(0, Fbo::BIND_DEPTHTEXTURE, shadowMapTexture);
    //shadowMapFbo->bindRenderBuffer(1, Fbo::BIND_DEPTHBUFFER, width, height);

    if (!shadowMapFbo->isOk()) {
        std::cerr << "ERROR: Shadow FBO setup failed" << std::endl;
        return false;
    }
    std::cout << "DEBUG: Shadow FBO setup successful, handle: "
        << shadowMapFbo->getHandle() << ", size "
        << shadowMapFbo->getSizeX() << "x" << shadowMapFbo->getSizeY() << std::endl;

    // Orthogonal projection matrix for shadow projection
    // range ideally should be calculated based on the camera frustum, but here it is
    // hard coded for prototyping
    float halfRange = range * 0.5f;
    lightProjectionMatrix = glm::ortho(-halfRange, halfRange, -halfRange, halfRange, -halfRange, halfRange);
    //glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    //glm::mat4 lightView = glm::lookAt(-lightDir * range * 0.5f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //lightSpaceMatrix = lightProjection * lightView;

    std::cout << "DEBUG: Light space matrix computed with orthographic range: ±" << halfRange << std::endl;

    // Ripristina lo stato di default
    Fbo::disable();

    // Activate the correct texture unit based on the shader manager parameters
    glActiveTexture(GL_TEXTURE0 + ShaderManager::SHADOW_MAP_UNIT);
    // Bind the texture to the current OpenGL context in the given unit.
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

    return true;
}

/**
 * @brief Renders all nodes in the render list.
 *
 * This method iterates through all nodes, computes their model-view matrices,
 * and invokes their render methods.
 *
 */
void Eng::List::renderNew() {
    glEnable(GL_DEPTH_TEST);
    auto& sm = ShaderManager::getInstance();

    // Render base color
    if (!sm.loadProgram(baseColorProgram)) {
        std::cerr << "ERROR: Failed to load base color program" << std::endl;
        return;
    }
    renderPass(false);

    // Render lights contribution
    for (int i = 0; i < lightsCount; ++i) {
        auto light = elements[i]->getNode();
        if (std::dynamic_pointer_cast<Eng::SpotLight>(light)) {
            if (!sm.loadProgram(spotLightProgram)) {
                std::cerr << "ERROR: Failed to load spot light program" << std::endl;
                return;
            }
        }
        else if (std::dynamic_pointer_cast<Eng::PointLight>(light)) {
            if (!sm.loadProgram(pointLightProgram)) {
                std::cerr << "ERROR: Failed to load point light program" << std::endl;
                return;
            }
        }
        else if (auto dirLight = std::dynamic_pointer_cast<Eng::DirectionalLight>(light)) {

            shadowPass(dirLight);

            if (!sm.loadProgram(dirLightProgram)) {
                std::cerr << "ERROR: Failed to load directional light program" << std::endl;
                return;
            }
            // Hard coded for always casting shadows with directional lights
            sm.setLightCastsShadows(true);
        }
        else {
            std::cerr << "ERROR: Unsupported light type" << std::endl;
            continue;
        }
        light->render();
        renderPass(true);
    }

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}
