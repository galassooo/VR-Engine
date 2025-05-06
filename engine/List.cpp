#include "engine.h"

// GLEW
#include <GL/glew.h>

#include <GL/freeglut.h>

#define SHADOW_RANGE 100.0f

/**
 * @brief Constructs a new render List with default settings.
 *
 * Initializes the base Object, creates a culling sphere,
 * and sets the default list name.
 */
Eng::List::List() : Object(), cullingSphere(std::make_unique<Eng::List::CullingSphere>()) {
   name = "RenderList";
}

/**
 * @brief Default destructor for List.
 */
Eng::List::~List() = default;


/**
 * @brief Constructs a culling sphere helper struct.
 *
 * Initializes center at origin and a unit radius.
 */
struct Eng::List::CullingSphere {
    glm::vec3 center;
    float radius;

    /**
    * Constructor.
    */
    CullingSphere() : center{ glm::vec3(0.0f) }, radius{ 1.0f } {
    }

    /**
     * @brief Destructor for CullingSphere.
     */
    ~CullingSphere() { }
};


/**
 * @brief Computes and returns the scene's axis-aligned bounding box.
 *
 * On first invocation, iterates over all mesh elements (excluding lights),
 * updates a BoundingBox, and logs corner coordinates.
 *
 * @return Shared pointer to the scene's BoundingBox.
 */
std::shared_ptr<Eng::BoundingBox> Eng::List::getSceneBoundingBox() {
    // The scene bounding box is computed only once at first call
	if (!sceneBoundingBox) {
		sceneBoundingBox = std::make_shared<Eng::BoundingBox>();
        std::cout << "[List] Computing Scene Bounding Box" << std::endl;
        for (int i = lightsCount; i < elements.size(); ++i) {

            if (const auto& mesh = dynamic_cast<Eng::Mesh*>(elements[i]->getNode().get())) {
                sceneBoundingBox->update(glm::vec3(mesh->getFinalMatrix() * glm::vec4(mesh->getBoundingBoxMin(), 1.0f)));
                sceneBoundingBox->update(glm::vec3(mesh->getFinalMatrix() * glm::vec4(mesh->getBoundingBoxMax(), 1.0f)));
            }
        }
        for (auto& vertex : sceneBoundingBox->getVertices()) {
            std::cout << "  (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ")" << std::endl;
        }
	}
	return sceneBoundingBox;
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
   currentFrustumCorners = nullptr;
   cullingSphere = nullptr;
}

/**
 * @brief Checks if a mesh is within the culling sphere.
 *
 * This method computes the distance between the mesh's bounding sphere and the
 * culling sphere. If the mesh is outside the culling sphere, it will not be rendered.
 *
 * @param mesh A shared pointer to the mesh being checked.
 * @return true if the mesh is within the culling sphere, false otherwise.
 */
bool Eng::List::isWithinCullingSphere(const std::shared_ptr<Eng::Mesh>& mesh) {
    // The mesh stores its bounding sphere (in local space).
    glm::vec3 localCenter = mesh->getBoundingSphereCenter();
    float localRadius = mesh->getBoundingSphereRadius();

    // Transform the bounding sphere center to eye space.
    glm::mat4 modelViewMatrix = eyeViewMatrix * mesh->getFinalMatrix();
    glm::vec3 eyeCenter = glm::vec3(modelViewMatrix * glm::vec4(localCenter, 1.0f));

    // For non-uniform scaling, extract an approximate uniform scale:
    float scale = glm::length(glm::vec3(modelViewMatrix[0]));
    float effectiveRadius = localRadius * scale;

    // Compute the squared distance from the transformed center to the culling sphere center.
    glm::vec3 diff = eyeCenter - cullingSphere->center;
    float distSq = glm::dot(diff, diff);
    float sumRadii = effectiveRadius + cullingSphere->radius;

    // If the object's sphere is completely outside, skip rendering it.
    if (distSq > (sumRadii * sumRadii))
        return false;
    else
        return true;
}

/**
 * @brief Recomputes the culling sphere from the view frustum corners.
 *
 * Builds a BoundingBox in eye-space from frustum corners, then sets
 * the sphere center and radius to tightly enclose that box.
 */
void Eng::List::updateCullingSphere() {
    // Virtual Environment
   // Here we do the sphere culling for the virtual environment as it is simpler to work in eye coordinates rather than world coordinates
   // The previous implementation was on the engine traverseAndAdd but it was wrong as we were mixing eye coordinates with world coordinates
    // Culling setup
    cullingSphere = std::make_unique<Eng::List::CullingSphere>();
    Eng::BoundingBox viewBoundingBox = Eng::BoundingBox();
    // Compute the view bounding box based on the frustum corners
    for (const auto& corner : getEyeFrustumCorners()) {
        viewBoundingBox.update(corner);
    }
    cullingSphere->center = viewBoundingBox.getCenter();
    cullingSphere->radius = glm::length(viewBoundingBox.getSize()) * 0.5f;
}

/**
 * @brief Performs the full multi-pass rendering of this list.
 *
 * Renders opaque geometry, then for each light renders its pass including
 * additive blending or shadow passes for directional lights.
 */
void Eng::List::render() {
	
    if (!cullingSphere) {
		updateCullingSphere();
	}

    glEnable(GL_DEPTH_TEST);
    auto& sm = ShaderManager::getInstance();

    // Render base color, without blending and with culling
    if (!sm.loadProgram(baseColorProgram)) {
        std::cerr << "ERROR: Failed to load base color program" << std::endl;
        return;
    }
    renderPass(false, true);

    // Render lights contribution, with blending and with culling
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

            // FBO should already be restored by shadowPass

            // Activate the correct texture unit based on the shader manager parameters
            glActiveTexture(GL_TEXTURE0 + ShaderManager::SHADOW_MAP_UNIT);
            // Bind the texture to the current OpenGL context in the given unit.
            glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

            if (!sm.loadProgram(dirLightProgram)) {
                std::cerr << "ERROR: Failed to load directional light program" << std::endl;
                return;
            }
            sm.setLightCastsShadows(true);
        }
        else {
            std::cerr << "ERROR: Unsupported light type" << std::endl;
            continue;
        }

        light->render();
        renderPass(true, true);
        renderTransparentPass();
    }

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

/**
 * @brief Renders transparent objects after lighting contributions.
 */
void Eng::List::renderTransparentPass()
{
    auto& sm = ShaderManager::getInstance();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);          // non scriviamo z
    glDepthFunc(GL_LEQUAL);

    for (int i = lightsCount; i < elements.size(); ++i) {
        if (elements[i]->getLayer() != RenderLayer::Transparent) continue;

        // === le stesse 6 righe di uniform che usi in renderPass() ===
        sm.setGlobalLightColor(globalLightColor);
        sm.setProjectionMatrix(eyeProjectionMatrix);

        glm::mat4 model = elements[i]->getWorldCoordinates();
        glm::mat4 mv = eyeViewMatrix * model;
        sm.setModelViewMatrix(mv);
        sm.setNormalMatrix(glm::inverseTranspose(glm::mat3(mv)));
        sm.setLightSpaceMatrix(lightSpaceMatrix * model);
        sm.setEyeFront(-glm::vec3(glm::transpose(glm::mat3(eyeViewMatrix))[2]));

        elements[i]->getNode()->render();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}


/**
 * @brief Renders the elements in the list.
 *
 * This method is called during the rendering process to draw all elements
 * in the list. It handles both additive and non-additive rendering.
 *
 * @param isAdditive A boolean indicating whether to use additive rendering.
 */
void Eng::List::renderPass(bool isAdditive, bool useCulling) {
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
        if (!isAdditive && elements[i]->getLayer() == RenderLayer::Transparent)
            continue;
        if (useCulling) {
			if (const auto& mesh = std::dynamic_pointer_cast<Mesh>(elements[i]->getNode())) {
				if (!isWithinCullingSphere(mesh))
					continue;
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

    // Reset previous OpenGL blending state
    if (blendingEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(srcRGB, dstRGB);
    }
    else {
        glDisable(GL_BLEND);
    }
}

/**
 * @brief Performs a depth-only shadow pass for a directional light.
 *
 * Sets up shadow map FBO, computes light-space matrices, and renders
 * scene depth into the shadow map. Restores previous FBO and viewport.
 *
 * @param light Shared pointer to the directional light.
 */
void Eng::List::shadowPass(std::shared_ptr <Eng::DirectionalLight>& light) {
    auto& sm = ShaderManager::getInstance();

    const auto& boundingBox = getSceneBoundingBox();

    // Store current viewport and FBO
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLint currentFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

    sm.loadProgram(shadowMapProgram);

    std::vector<glm::vec3> cameraFrustumCorners = computeFrustumCorners(eyeProjectionMatrix, eyeViewMatrix);

    std::vector<glm::vec3> boundingBoxVertices = boundingBox->getVertices();

    glm::mat4 lightViewMatrix = light->getLightViewMatrix(cameraFrustumCorners, glm::length(boundingBox->getSize()));

	glm::mat4 lightProjectionMatrix = computeLightProjectionMatrix(lightViewMatrix, boundingBoxVertices);


    // calculate and set the lightSpaceMatrix for shadow projection
    lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;

    // Activate and clean the shadow map FBO
    shadowMapFbo->render();
    glViewport(0, 0, shadowMapFbo->getSizeX(), shadowMapFbo->getSizeY());

    // Enable depth writing
    glDepthMask(GL_TRUE);

    // Clear depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // Run the render pass (no additive and no culling)
    renderPass(false, false);

    // IMPORTANT: Restore the previous FBO and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

/**
 * @brief Computes the frustum corners based on the current view and projection matrices.
 *
 * The frustum corners are computed in world coordinates.
 * If they are already computed, they are returned from the cache.
 *
 * @param projectionMatrix The projection matrix.
 * @param viewMatrix The view matrix.
 * @return A vector of 8 corners representing the frustum.
 */
std::vector<glm::vec3> Eng::List::getEyeFrustumCorners() {
    // The frustum corners are cached once computed
    if (!currentFrustumCorners) {
        currentFrustumCorners = std::make_unique<std::vector<glm::vec3>>(computeFrustumCorners(eyeProjectionMatrix, eyeViewMatrix));
    }
    return *currentFrustumCorners;
}

/**
 * @brief Computes the light projection matrix based on the light view matrix and the scene bounding box corners.
 *
 * This method transforms the bounding box corners to light space and computes the
 * orthographic projection matrix based on the light space bounding box.
 *
 * @param lightViewMatrix The view matrix of the light source.
 * @param boundingBoxVertices The corners of the scene bounding box in world coordinates.
 * @return The computed orthographic projection matrix for the light source.
 */
glm::mat4 Eng::List::computeLightProjectionMatrix(const glm::mat4& lightViewMatrix, const std::vector<glm::vec3>& boundingBoxVertices) {
    std::vector<glm::vec3> lightSpaceVertices;
    for (const auto& vertex : boundingBoxVertices) {
        glm::vec4 transformed = lightViewMatrix * glm::vec4(vertex, 1.0f);
        lightSpaceVertices.push_back(glm::vec3(transformed));
    }

    glm::vec3 minLS(FLT_MAX);
    glm::vec3 maxLS(-FLT_MAX);

    for (const auto& v : lightSpaceVertices) {
        minLS = glm::min(minLS, v);
        maxLS = glm::max(maxLS, v);
    }

	// Build the ortho projection matrix based on the light space bounding box
    glm::mat4 lightProjection = glm::ortho(
        minLS.x, maxLS.x,
        minLS.y, maxLS.y,
        -maxLS.z, -minLS.z // OpenGL NDC depth goes in the opposite direction -1 to 1
    );

    return lightProjection;
}

/**
 * @brief Computes world-space view frustum corners from projection and view matrices.
 *
 * Maps NDC corners back through the inverse view-projection transform.
 *
 * @param projectionMatrix Current projection matrix.
 * @param viewMatrix       Current view matrix.
 * @return Vector of 8 world-space frustum corner positions.
 */
std::vector<glm::vec3> Eng::List::computeFrustumCorners(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
    std::vector<glm::vec3> corners = {
        glm::vec3(-1, -1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(1,  1, -1),
        glm::vec3(-1,  1, -1),
        glm::vec3(-1, -1,  1),
        glm::vec3(1, -1,  1),
        glm::vec3(1,  1,  1),
        glm::vec3(-1,  1,  1)
    };

    glm::mat4 invViewProj = glm::inverse(projectionMatrix * viewMatrix);

    for (auto& corner : corners) {
        glm::vec4 corner4(corner, 1.0f);
        corner4 = invViewProj * corner4;
        corner = glm::vec3(corner4 / corner4.w); // normalize the omogeneous coordinates
    }

    return corners;
}

/**
 * @brief Retrieves all elements in the list.
 * @return A vector of shared pointers to the list elements in the list.
 */
std::vector<std::shared_ptr<Eng::ListElement> > Eng::List::getElements() const {
   return elements;
}

/**
 * @brief Set new eye view matrix in the render list.
 * @param glm::mat4 Eye view matrix.
 */
void Eng::List::setEyeViewMatrix(glm::mat4& viewMatrix) {
    this->eyeViewMatrix = viewMatrix;
	cullingSphere = nullptr; // Reset the culling sphere cache
	currentFrustumCorners = nullptr; // Reset the frustum corners cache
}

/**
* @brief Set new eye projection matrix in the render list.
* @param glm::mat4 Eye projection matrix.
*/
void Eng::List::setEyeProjectionMatrix(glm::mat4& eyeProjectionMatrix) {
	this->eyeProjectionMatrix = eyeProjectionMatrix;
    cullingSphere = nullptr; // Reset the culling sphere cache
	currentFrustumCorners = nullptr; // Reset the frustum corners cache
}

/**
 * @brief Sets the global light color for the render list.
 *
 * This method updates the global light color used in the rendering process.
 *
 * @param globalColor The new global light color as a glm::vec3.
 */
void Eng::List::setGlobalLightColor(const glm::vec3& globalColor) {
    this->globalLightColor = globalColor;
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
bool Eng::List::setupShadowMap(int width, int height) {
    std::cout << "DEBUG: [List] Setting up shadow map, requested size: "
        << width << "x" << height << std::endl;

    if (width <= 0 || height <= 0) {
        width = 1024;
        height = 1024;
        std::cout << "DEBUG: [List] Using default shadow map size and range: "
            << width << "x" << height << std::endl;
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
    std::cout << "DEBUG: [List] Shadow depth texture created with ID: " << shadowMapTexture << std::endl;

    // Bind texture all'FBO
    shadowMapFbo->bindTexture(0, Fbo::BIND_DEPTHTEXTURE, shadowMapTexture);
    //shadowMapFbo->bindRenderBuffer(1, Fbo::BIND_DEPTHBUFFER, width, height);

    if (!shadowMapFbo->isOk()) {
        std::cerr << "ERROR: Shadow FBO setup failed" << std::endl;
        return false;
    }
    std::cout << "DEBUG: [List] Shadow FBO setup successful, handle: "
        << shadowMapFbo->getHandle() << ", size "
        << shadowMapFbo->getSizeX() << "x" << shadowMapFbo->getSizeY() << std::endl;

    // Ripristina lo stato di default
    Fbo::disable();
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

/**
 * @brief Initializes the shaders used in the render list.
 *
 * This method loads and compiles the vertex and fragment shaders for basic rendering,
 * shadow mapping, and light contributions. It also sets up the shader programs for use.
 *
 * @return true if shaders are initialized successfully, false otherwise.
 */
bool Eng::List::initShaders() {
    static bool initialized = false;
    if (initialized) return true;

    // Set up the shadow map framebuffer object (FBO)
    if (!setupShadowMap(2048, 2048))
        return false;

    /**************** Basic vertex shader *****************/
    const std::string basicVertexCode = R"(
   #version 440 core

   // Uniforms
   uniform mat4 ShaderManager::UNIFORM_PROJECTION_MATRIX;
   uniform mat4 ShaderManager::UNIFORM_MODELVIEW_MATRIX;
   uniform mat3 ShaderManager::UNIFORM_NORMAL_MATRIX;

   // Attributes
   layout(location = ShaderManager::POSITION_LOCATION) in vec3 in_Position;
   layout(location = ShaderManager::NORMAL_LOCATION) in vec3 in_Normal;
   layout(location = ShaderManager::TEX_COORD_LOCATION) in vec2 in_TexCoord;  // Aggiunto per texture

   // Varying (Passing to fragment shader):
   out vec4 fragPos;
   out vec3 fragNormal;
   out vec2 texCoord;  // Aggiunto per texture

   void main(void)
   {
      // 1) Transform the incoming vertex position to eye space:
      fragPos = ShaderManager::UNIFORM_MODELVIEW_MATRIX * vec4(in_Position, 1.0);

      // 2) Transform to clip space by applying the projection.
      gl_Position = ShaderManager::UNIFORM_PROJECTION_MATRIX * fragPos;

      // 3) Transform the normal from object space into eye space
      fragNormal = ShaderManager::UNIFORM_NORMAL_MATRIX * in_Normal;
      
      // 4) Pass texture coordinates to fragment shader
      texCoord = in_TexCoord;
   }
)";

    basicVertexShader = std::make_shared<Eng::VertexShader>();
    basicVertexShader->load(ShaderManager::preprocessShaderCode(basicVertexCode).c_str());

    /**************** Shadow Mapping vertex shader *****************/
    const std::string shadowMapVertexCode = R"(
#version 440 core
layout (location = ShaderManager::POSITION_LOCATION) in vec3 aPos;

uniform mat4 ShaderManager::UNIFORM_LIGHTSPACE_MATRIX; // in this case from the view of the light

void main()
{
    gl_Position = ShaderManager::UNIFORM_LIGHTSPACE_MATRIX * vec4(aPos, 1.0);
}
)";

    shadowMapVertexShader = std::make_shared<Eng::VertexShader>();
    shadowMapVertexShader->load(ShaderManager::preprocessShaderCode(shadowMapVertexCode).c_str());

    /**************** Shadow Mapping fragment shader *****************/
    const std::string shadowMapFragmentCode = R"(
#version 330 core
void main() {
    // Empty because it only writes in depth buffer
}

)";

    shadowMapFragmentShader = std::make_shared<Eng::FragmentShader>();
    shadowMapFragmentShader->load(shadowMapFragmentCode.c_str());

    /**************** Base Color fragment shader *****************/
    const std::string baseFragmentCode = R"(
   #version 440 core
    
   // Varying variables from vertex shader
   in vec4 fragPos;
   in vec3 fragNormal;
   in vec2 texCoord;  // Aggiunto per texture

   out vec4 fragOutput; // Final color to render

   // Material properties:
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_EMISSION;

   // Global properties:
   uniform vec3 ShaderManager::UNIFORM_GLOBAL_LIGHT_COLOR;

   // Eye properties
   uniform vec3 ShaderManager::UNIFORM_EYE_FRONT;

   // Texture mapping:
   layout(binding = ShaderManager::DIFFUSE_TEXTURE_UNIT) uniform sampler2D texSampler;
   uniform bool ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE;  // Flag per indicare se usare la texture

   void main(void)
   {
      // Emission only
      vec3 color = ShaderManager::UNIFORM_MATERIAL_EMISSION;

      //fragOutput = vec4(1.0, 0.0, 0.0, 1.0); // rosso opaco per debug

      // Global specular contribution based on the normal's tilt relative to the horizontal plane

      // Direction from texel to eye in eye-space
      vec3 V = normalize(-fragPos.xyz);
      // Interpolated normal form the vertex shader in eye-space
      vec3 N = normalize(fragNormal);

      float globalSpecStrength = 0.8; // controls how strong the global specular is

      // 1. How much the normal is perpendicular to the view direction
      float normalViewAlignment = abs(dot(N, V));
      float perpendFactor = 1.0 - normalViewAlignment;

      // 2. How much the camera is tilted relative to the horizontal plane
      float cameraInclination = dot(normalize(ShaderManager::UNIFORM_EYE_FRONT), vec3(0.0, 1.0, 0.0));
      float horizonFactor = (cameraInclination >= 0.0) ? 1.0 : 1.0 + cameraInclination;

      // 3. Combine the two factors
      float globalSpecFactor = perpendFactor * horizonFactor;

      // Raise to a power to controll the falloff
      //globalSpecFactor = pow(globalSpecFactor, 2.0);

      // Add global specular contribution
      color += ShaderManager::UNIFORM_GLOBAL_LIGHT_COLOR * globalSpecFactor * globalSpecStrength;


      // Final color calculation with texture
      if (ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE) {
         vec4 texColor = texture(texSampler, texCoord);
         fragOutput = vec4(color, 1.0) * texColor;
      } else {
         fragOutput = vec4(color, 1.0);
      }
  
   }
)";
    basicFragmentShader = std::make_shared<Eng::FragmentShader>();
    basicFragmentShader->load(ShaderManager::preprocessShaderCode(baseFragmentCode).c_str());


    /**************** Point Light fragment shader *****************/

    const std::string pointLightFragmentCode = R"(
#version 440 core
    
   // Varying variables from vertex shader
   in vec4 fragPos;
   in vec3 fragNormal;
   in vec2 texCoord;  // Aggiunto per texture

   out vec4 fragOutput; // Final color to render

   // Material properties:
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_AMBIENT;
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_DIFFUSE;
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_SPECULAR;
   uniform float ShaderManager::UNIFORM_MATERIAL_SHININESS;

   // Light properties
   uniform vec3 ShaderManager::UNIFORM_LIGHT_POSITION;
   uniform vec3 ShaderManager::UNIFORM_LIGHT_AMBIENT;
   uniform vec3 ShaderManager::UNIFORM_LIGHT_DIFFUSE;
   uniform vec3 ShaderManager::UNIFORM_LIGHT_SPECULAR;
   uniform float ShaderManager::UNIFORM_ATTENUATION_CONSTANT;
   uniform float ShaderManager::UNIFORM_ATTENUATION_LINEAR;
   uniform float ShaderManager::UNIFORM_ATTENUATION_QUADRATIC;
   
   // Texture mapping:
   layout(binding = ShaderManager::DIFFUSE_TEXTURE_UNIT) uniform sampler2D texSampler;
   uniform bool ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE;  // Flag per indicare se usare la texture

   void main(void)
   {
      // Ambient
      vec3 color = ShaderManager::UNIFORM_MATERIAL_AMBIENT * ShaderManager::UNIFORM_LIGHT_AMBIENT;

      // Interpolated normal form the vertex shader
      vec3 N = normalize(fragNormal);

      // Light direction in eye-space
      vec3 L = ShaderManager::UNIFORM_LIGHT_POSITION - fragPos.xyz;
      float distance = length(L);
      L = normalize(L);
      
      // Light Attenuation
      float attenuation = 1.0 / (
      ShaderManager::UNIFORM_ATTENUATION_CONSTANT +
      ShaderManager::UNIFORM_ATTENUATION_LINEAR * distance +
      ShaderManager::UNIFORM_ATTENUATION_QUADRATIC * (distance * distance)
      );

      // Lambert's cosine term
      float lambert = max(dot(N, L), 0.0);

      if (lambert > 0.0)
      {
         // Add diffuse contribution
         color += ShaderManager::UNIFORM_MATERIAL_DIFFUSE * lambert * ShaderManager::UNIFORM_LIGHT_DIFFUSE * attenuation;

         // Blinn-Phong specular
         vec3 V = normalize(-fragPos.xyz);
         vec3 H = normalize(L + V);

         float specAngle = max(dot(N, H), 0.0);
         color += ShaderManager::UNIFORM_MATERIAL_SPECULAR * pow(specAngle, ShaderManager::UNIFORM_MATERIAL_SHININESS) * ShaderManager::UNIFORM_LIGHT_SPECULAR * attenuation;
      }
      
      // Final color calculation with texture
      if (ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE) {
         vec4 texColor = texture(texSampler, texCoord);
         fragOutput = vec4(color, 1.0) * texColor;
      } else {
         fragOutput = vec4(color, 1.0);
      }
   }
)";

    pointFragmentShader = std::make_shared<Eng::FragmentShader>();
    pointFragmentShader->load(ShaderManager::preprocessShaderCode(pointLightFragmentCode).c_str());

    /**************** Spot Light fragment shader *****************/

    const std::string spotLightFragmentCode = R"(
#version 440 core
    
   // Varying variables from vertex shader
   in vec4 fragPos;
   in vec3 fragNormal;
   in vec2 texCoord;  // Aggiunto per texture

   out vec4 fragOutput; // Final color to render

   // Material properties:
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_AMBIENT;
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_DIFFUSE;
   uniform vec3 ShaderManager::UNIFORM_MATERIAL_SPECULAR;
   uniform float ShaderManager::UNIFORM_MATERIAL_SHININESS;

   // Light properties
   uniform vec3 ShaderManager::UNIFORM_LIGHT_POSITION;
   uniform vec3 ShaderManager::UNIFORM_LIGHT_DIRECTION;
   uniform float ShaderManager::UNIFORM_LIGHT_CUTOFF_ANGLE;  // in degrees
   uniform float ShaderManager::UNIFORM_LIGHT_FALLOFF;       // like GL_SPOT_EXPONENT
   uniform vec3 ShaderManager::UNIFORM_LIGHT_AMBIENT;
   uniform vec3 ShaderManager::UNIFORM_LIGHT_DIFFUSE;
   uniform vec3 ShaderManager::UNIFORM_LIGHT_SPECULAR;
   uniform float ShaderManager::UNIFORM_ATTENUATION_CONSTANT;
   uniform float ShaderManager::UNIFORM_ATTENUATION_LINEAR;
   uniform float ShaderManager::UNIFORM_ATTENUATION_QUADRATIC;
   
   // Texture mapping:
   layout(binding = ShaderManager::DIFFUSE_TEXTURE_UNIT) uniform sampler2D texSampler;
   uniform bool ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE;  // Flag per indicare se usare la texture

   void main(void)
   {
      // Ambient
      vec3 color = ShaderManager::UNIFORM_MATERIAL_AMBIENT * ShaderManager::UNIFORM_LIGHT_AMBIENT;

      // Interpolated normal form the vertex shader
      vec3 N = normalize(fragNormal);

      // Light direction in eye-space
      vec3 L = ShaderManager::UNIFORM_LIGHT_POSITION - fragPos.xyz;
      float distance = length(L);
      L = normalize(L);
      
      // Light Attenuation
      float attenuation = 1.0 / (
        ShaderManager::UNIFORM_ATTENUATION_CONSTANT +
        ShaderManager::UNIFORM_ATTENUATION_LINEAR * distance +
        ShaderManager::UNIFORM_ATTENUATION_QUADRATIC * (distance * distance)
      );

      // Spot Light intensity
      vec3 spotDir = normalize(-ShaderManager::UNIFORM_LIGHT_DIRECTION);
      float cosTheta = dot(L, spotDir); // Defines how much the fragment is within the light cone

      // compare with cosine of cutoff in radians
      float cutoffRadians = radians(ShaderManager::UNIFORM_LIGHT_CUTOFF_ANGLE);
      float cutoffCos = cos(cutoffRadians); // Threshold for beginning of light falloff

      // Added for soft spot light
      // Maximum threshold for the outer cone
      float outerCutoff = cos(radians(ShaderManager::UNIFORM_LIGHT_CUTOFF_ANGLE + ShaderManager::UNIFORM_LIGHT_FALLOFF));

      float intensity = clamp((cosTheta - outerCutoff) / (cutoffCos - outerCutoff), 0.0, 1.0);
      // Old formula - Spotlight falloff (exponential)
      //if (cosTheta > cutoffCos) {
      //  intensity = pow(cosTheta, ShaderManager::UNIFORM_LIGHT_FALLOFF);
      //}

      // Lambert's cosine term
      float lambert = max(dot(N, L), 0.0);

      if (lambert > 0.0)
      {
         // Add diffuse contribution
         color += ShaderManager::UNIFORM_MATERIAL_DIFFUSE * lambert * ShaderManager::UNIFORM_LIGHT_DIFFUSE * attenuation * intensity;

         // Blinn-Phong specular
         vec3 V = normalize(-fragPos.xyz);
         vec3 H = normalize(L + V);

         float specAngle = max(dot(N, H), 0.0);
         color += ShaderManager::UNIFORM_MATERIAL_SPECULAR * pow(specAngle, ShaderManager::UNIFORM_MATERIAL_SHININESS) * 
                  ShaderManager::UNIFORM_LIGHT_SPECULAR * attenuation * intensity;
      }
      
      // Final color calculation with texture
      if (ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE) {
         vec4 texColor = texture(texSampler, texCoord);
         fragOutput = vec4(color, 1.0) * texColor;
      } else {
         fragOutput = vec4(color, 1.0);
      }
   }
)";
    spotFragmentShader = std::make_shared<Eng::FragmentShader>();
    spotFragmentShader->load(ShaderManager::preprocessShaderCode(spotLightFragmentCode).c_str());

    /**************** Directional Light vertex shader *****************/
    const std::string dirLightVertexCode = R"(
#version 440 core

// Uniforms
uniform mat4 ShaderManager::UNIFORM_PROJECTION_MATRIX;
uniform mat4 ShaderManager::UNIFORM_MODELVIEW_MATRIX;
uniform mat3 ShaderManager::UNIFORM_NORMAL_MATRIX;
uniform mat4 ShaderManager::UNIFORM_LIGHTSPACE_MATRIX; // Nuovo: trasforma verso light-space

// Attributes
layout(location = ShaderManager::POSITION_LOCATION) in vec3 in_Position;
layout(location = ShaderManager::NORMAL_LOCATION) in vec3 in_Normal;
layout(location = ShaderManager::TEX_COORD_LOCATION) in vec2 in_TexCoord;

// Varying (verso il fragment shader)
out vec4 fragPos;
out vec3 fragNormal;
out vec2 texCoord;
out vec4 fragPosLightSpace; // Nuovo: posizione nel light-space

void main(void)
{
   // 1) Transform into eye space
   fragPos = ShaderManager::UNIFORM_MODELVIEW_MATRIX * vec4(in_Position, 1.0);

   // 2) Transform into clip space
   gl_Position = ShaderManager::UNIFORM_PROJECTION_MATRIX * fragPos;

   // 3) Normal transformed into eye space
   fragNormal = ShaderManager::UNIFORM_NORMAL_MATRIX * in_Normal;

   // 4) Passing through texture coordinates
   texCoord = in_TexCoord;

   // 5) Computing light-space coordinates of the vertex
   fragPosLightSpace = ShaderManager::UNIFORM_LIGHTSPACE_MATRIX * vec4(in_Position, 1.0);
}

)";
    dirLightVertexShader = std::make_shared<Eng::VertexShader>();
    dirLightVertexShader->load(ShaderManager::preprocessShaderCode(dirLightVertexCode).c_str());

    /**************** Directional Light fragment shader *****************/

    const std::string dirLightFragmentCode = R"(
#version 440 core

// Varying variables from vertex shader
in vec4 fragPos;
in vec3 fragNormal;
in vec2 texCoord;
in vec4 fragPosLightSpace;

out vec4 fragOutput;

// Material properties
uniform vec3 ShaderManager::UNIFORM_MATERIAL_AMBIENT;
uniform vec3 ShaderManager::UNIFORM_MATERIAL_DIFFUSE;
uniform vec3 ShaderManager::UNIFORM_MATERIAL_SPECULAR;
uniform float ShaderManager::UNIFORM_MATERIAL_SHININESS;

// Light properties
uniform vec3 ShaderManager::UNIFORM_LIGHT_DIRECTION; // already normalized
uniform vec3 ShaderManager::UNIFORM_LIGHT_AMBIENT;
uniform vec3 ShaderManager::UNIFORM_LIGHT_DIFFUSE;
uniform vec3 ShaderManager::UNIFORM_LIGHT_SPECULAR;
uniform bool ShaderManager::UNIFORM_LIGHT_CASTS_SHADOWS;

// Shadow mapping
layout(binding = ShaderManager::SHADOW_MAP_UNIT) uniform sampler2D shadowMap;

// Texture mapping
layout(binding = ShaderManager::DIFFUSE_TEXTURE_UNIT) uniform sampler2D texSampler;
uniform bool ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE;

float computeShadowFactor(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.001);
    return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
}

void main(void)
{
    vec3 color = ShaderManager::UNIFORM_MATERIAL_AMBIENT * ShaderManager::UNIFORM_LIGHT_AMBIENT;

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-ShaderManager::UNIFORM_LIGHT_DIRECTION); // direction towards the light

    float lambert = max(dot(N, L), 0.0);

    if (lambert > 0.0)
    {
        float shadow = 0.0;
        if (ShaderManager::UNIFORM_LIGHT_CASTS_SHADOWS)
            shadow = computeShadowFactor(fragPosLightSpace, N, L);

        float lightFactor = 1.0 - shadow;

        color += ShaderManager::UNIFORM_MATERIAL_DIFFUSE * lambert * ShaderManager::UNIFORM_LIGHT_DIFFUSE * lightFactor;

        vec3 V = normalize(-fragPos.xyz);
        vec3 H = normalize(L + V);
        float specAngle = max(dot(N, H), 0.0);
        color += ShaderManager::UNIFORM_MATERIAL_SPECULAR * pow(specAngle, ShaderManager::UNIFORM_MATERIAL_SHININESS) * ShaderManager::UNIFORM_LIGHT_SPECULAR * lightFactor;
    }

    if (ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE) {
        vec4 texColor = texture(texSampler, texCoord);
        fragOutput = vec4(color, 1.0) * texColor;
    } else {
        fragOutput = vec4(color, 1.0);
    }
}
)";
    directionalFragmentShader = std::make_shared<Eng::FragmentShader>();
    directionalFragmentShader->load(ShaderManager::preprocessShaderCode(dirLightFragmentCode).c_str());


    //Compile and link Basic Shaders used for the first pass
    baseColorProgram = std::make_shared<Eng::Program>();
    baseColorProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    baseColorProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler");
    if (!baseColorProgram->addShader(basicFragmentShader).addShader(basicVertexShader).build())
        return false;

    //Compile and link Shaders used for the Shadow Mapping pass
    shadowMapProgram = std::make_shared<Eng::Program>();
    shadowMapProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "aPos");
    if (!shadowMapProgram->addShader(shadowMapFragmentShader).addShader(shadowMapVertexShader).build())
        return false;

    //Compile and link Point light pass program
    pointLightProgram = std::make_shared<Eng::Program>();
    pointLightProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    pointLightProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler");
    if (!pointLightProgram->addShader(pointFragmentShader).addShader(basicVertexShader).build())
        return false;

    //Compile and link Shaders used for the Spot light pass
    spotLightProgram = std::make_shared<Eng::Program>();
    spotLightProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    spotLightProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler");
    if (!spotLightProgram->addShader(spotFragmentShader).addShader(basicVertexShader).build())
        return false;

    //Compile and link Shaders used for the Directional light pass
    dirLightProgram = std::make_shared<Eng::Program>();
    dirLightProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    dirLightProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler").bindSampler(ShaderManager::SHADOW_MAP_UNIT, "shadowMap");
    if (!dirLightProgram->addShader(directionalFragmentShader).addShader(dirLightVertexShader).build())
        return false;

    initialized = true;
    return initialized;
}
