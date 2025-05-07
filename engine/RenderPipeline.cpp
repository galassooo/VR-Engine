#include "engine.h"

#include <GL/glew.h>

#define SHADOWMAP_WIDTH 2048
#define SHADOWMAP_HEIGHT 2048

// Helper stuct holding status cache for OpenGL state
struct Eng::RenderPipeline::StatusCache {
    bool depthTestEnabled = false;
    bool depthWriteEnabled = false;
    GLenum depthFunc = GL_LESS;
    bool blendingEnabled = false;
    GLint blendSrcRGB = GL_SRC_ALPHA;
    GLint blendDstRGB = GL_ONE_MINUS_SRC_ALPHA;
	GLint fbo = 0;
    GLint viewport[4];
};

// Helper struct holding render context
struct Eng::RenderPipeline::RenderContext {
	List* renderList;
    std::vector<RenderLayer> layers;
    bool useCulling;
    bool isAdditive;
	bool isTransparent;
};

/**
 * @brief Constructs a new RenderPipeline object.
 */
Eng::RenderPipeline::RenderPipeline() {
	// Initialize the status cache
	prevStatus = std::make_unique<StatusCache>();
}

/**
 * @brief Destructs the RenderPipeline object.
 */
Eng::RenderPipeline::~RenderPipeline() = default;

/**
 * @brief Performs a depth-only shadow pass for a directional light.
 *
 * Sets up shadow map FBO, computes light-space matrices, and renders
 * scene depth into the shadow map. Restores previous FBO and viewport.
 *
 * @param light Shared pointer to the directional light.
 */
void Eng::RenderPipeline::shadowPass(std::shared_ptr <Eng::DirectionalLight>& light, Eng::List* renderList) {
    auto& sm = ShaderManager::getInstance();

    const auto& boundingBox = renderList->getSceneBoundingBox();

    // Store current viewport and FBO
    glGetIntegerv(GL_VIEWPORT, prevStatus->viewport);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevStatus->fbo);

    sm.loadProgram(shadowMapProgram);

    std::vector<glm::vec3> cameraFrustumCorners = renderList->getEyeFrustumCorners();

    // calculate and set the lightSpaceMatrix for shadow projection
    lightSpaceMatrix = light->getLightSpaceMatrix(cameraFrustumCorners, boundingBox);

    // Activate and clean the shadow map FBO
    shadowMapFbo->render();
    glViewport(0, 0, shadowMapFbo->getSizeX(), shadowMapFbo->getSizeY());

    // Shadow pass context (no culling and no additive <-- writes depth)
	std::shared_ptr<RenderContext> context = std::make_shared<RenderContext>();
	context->renderList = renderList;
	context->layers = { RenderLayer::Opaque };
	context->useCulling = false;
	context->isAdditive = false;

    renderPass(context);

    // IMPORTANT: Restore the previous FBO and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, prevStatus->fbo);
    glViewport(prevStatus->viewport[0], prevStatus->viewport[1], 
        prevStatus->viewport[2], prevStatus->viewport[3]);
}

/**
 * @brief Runs the render pipeline on the provided render list.
 *
 * This method executes sequential rendering passes for the scene,
 * including base color, lighting, and shadow passes.
 * For each pass it sets up the render context and shader program.
 *
 */
void Eng::RenderPipeline::runOn(Eng::List* renderList) {
    glEnable(GL_DEPTH_TEST);
    auto& sm = ShaderManager::getInstance();

	// Set up the render context
	std::shared_ptr<RenderContext> context = std::make_shared<RenderContext>();
	context->renderList = renderList;

    // Base color pass

	context->layers = { RenderLayer::Opaque };
	context->useCulling = true;
	context->isAdditive = false;
	context->isTransparent = false;

	sm.loadProgram(baseColorProgram);

	renderPass(context);

	// Lighting pass

	ListIterator lightIterator = renderList->getLayerIterator(RenderLayer::Lights);
	std::shared_ptr<Node> light;
    while (lightIterator.hasNext()) {
        light = lightIterator.next()->getNode();
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
            shadowPass(dirLight, renderList);

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

		// Set up the render context for the light

		// First pass: render opaque objects
		context->layers = { RenderLayer::Opaque, RenderLayer::Transparent };
		context->useCulling = true;
		context->isAdditive = true;
		context->isTransparent = false;
        renderPass(context);

		// Second pass: render transparent objects
        //context->layers = { RenderLayer::Transparent };
        //context->isTransparent = true;
		//renderPass(context);
    }

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}


/**
 * @brief Renders the scene based on the provided render context.
 *
 * This method caches the current OpenGL state, sets up blending and depth
 * based on the provided context, iterates through the render list,
 * and renders each element in the specified layers.
 *
 * @param context A shared pointer to the RenderContext containing rendering parameters.
 */
void Eng::RenderPipeline::renderPass(const std::shared_ptr<RenderContext>& context) {
    auto& sm = ShaderManager::getInstance();

    // rembember current OpenGL state
    prevStatus->blendingEnabled = glIsEnabled(GL_BLEND);

    if (prevStatus->blendingEnabled) {
        glGetIntegerv(GL_BLEND_SRC_RGB, &prevStatus->blendSrcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &prevStatus->blendDstRGB);
    }

    if (context->isTransparent) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);          // non scriviamo z
        glDepthFunc(GL_LEQUAL);
    }
    else if (context->isAdditive) {
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

        // Clear depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);
    }


	for (const auto& layer : context->layers) {
        auto renderIterator = context->renderList->getLayerIterator(layer);
        std::shared_ptr<ListElement> element;

        while (renderIterator.hasNext()) {
			element = renderIterator.next();
            if (context->useCulling) {
                if (const auto& mesh = std::dynamic_pointer_cast<Mesh>(element->getNode())) {
                    if (!context->renderList->isWithinCullingSphere(mesh))
                        continue;
                }
            }

			glm::mat4 eyeViewMatrix = context->renderList->getEyeViewMatrix();

            // Load global light color
            sm.setGlobalLightColor(context->renderList->getGlobalLightColor());

            // Load projection matrix
            sm.setProjectionMatrix(context->renderList->getEyeProjectionMatrix());

            glm::mat4 modelMatrix = element->getWorldCoordinates();

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

            element->getNode()->render();
        }
	}

    // Reset previous OpenGL blending state
    if (prevStatus->blendingEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(prevStatus->blendSrcRGB, prevStatus->blendDstRGB);
    }
    else {
        glDisable(GL_BLEND);
    }
}

/**
 * @brief Sets up the shadow map framebuffer object (FBO) and texture.
 *
 * This method creates a depth-only FBO and a depth texture for shadow mapping.
 * It also configures the texture parameters and binds it to the FBO.
 *
 * @param width The width of the shadow map texture.
 * @param height The height of the shadow map texture.
 * @return true if setup is successful, false otherwise.
 */
bool Eng::RenderPipeline::setupShadowMap(int width, int height) {
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
 * @brief Initializes the render pipeline by setting up shaders and shadow map.
 *
 * This method loads the shaders, sets up the shadow map framebuffer object (FBO),
 * and prepares the rendering context for the pipeline.
 *
 * @return true if initialization is successful, false otherwise.
 */
bool Eng::RenderPipeline::init() {
	static bool initialized = false;
	if (initialized) return true;

	// Set up the shadow map framebuffer object (FBO)
	if (!setupShadowMap(SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT))
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