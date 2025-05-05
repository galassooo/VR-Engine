#include "engine.h"

#include <GL/glew.h>

//#include <GL/freeglut.h>

Eng::LightPass::LightPass(std::shared_ptr<Eng::VertexShader>& vs, std::shared_ptr<Eng::List> renderList)
    : renderList{ renderList }, vertexShader{ vs } 
{

}

void Eng::LightPass::setRenderList(std::shared_ptr<Eng::List>& renderList) {
    this->renderList = renderList;
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
bool Eng::LightPass::setupShadowMap(int width, int height) {
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

bool Eng::LightPass::init() {

    if (initialized)
        return true;

    // Set up the shadow map framebuffer object (FBO)
    if (!setupShadowMap(2048, 2048))
        return false;

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

    std::shared_ptr<Eng::FragmentShader> pointFragmentShader = std::make_shared<Eng::FragmentShader>();
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
    std::shared_ptr<Eng::FragmentShader> spotFragmentShader = std::make_shared<Eng::FragmentShader>();
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
    std::shared_ptr<Eng::VertexShader> dirLightVertexShader = std::make_shared<Eng::VertexShader>();
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
    std::shared_ptr<Eng::FragmentShader> directionalFragmentShader = std::make_shared<Eng::FragmentShader>();
    directionalFragmentShader->load(ShaderManager::preprocessShaderCode(dirLightFragmentCode).c_str());

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

    std::shared_ptr<Eng::VertexShader> shadowMapVertexShader = std::make_shared<Eng::VertexShader>();
    shadowMapVertexShader->load(ShaderManager::preprocessShaderCode(shadowMapVertexCode).c_str());

    /**************** Shadow Mapping fragment shader *****************/
    const std::string shadowMapFragmentCode = R"(
#version 330 core
void main() {
    // Empty because it only writes in depth buffer
}

)";

    std::shared_ptr<Eng::FragmentShader>shadowMapFragmentShader = std::make_shared<Eng::FragmentShader>();
    shadowMapFragmentShader->load(shadowMapFragmentCode.c_str());

    //Compile and link Point light pass program
    pointProgram = std::make_shared<Eng::Program>();
    pointProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    pointProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler");
    if (!pointProgram->addShader(pointFragmentShader).addShader(vertexShader).build())
        return false;

    //Compile and link Shaders used for the Spot light pass
    spotProgram = std::make_shared<Eng::Program>();
    spotProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    spotProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler");
    if (!spotProgram->addShader(spotFragmentShader).addShader(vertexShader).build())
        return false;

    //Compile and link Shaders used for the Directional light pass
    dirProgram = std::make_shared<Eng::Program>();
    dirProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    dirProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler").bindSampler(ShaderManager::SHADOW_MAP_UNIT, "shadowMap");
    if (!dirProgram->addShader(directionalFragmentShader).addShader(dirLightVertexShader).build())
        return false;

    //Compile and link Shaders used for the Shadow Mapping pass
    shadowMapProgram = std::make_shared<Eng::Program>();
    shadowMapProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "aPos");
    if (!shadowMapProgram->addShader(shadowMapFragmentShader).addShader(shadowMapVertexShader).build())
        return false;

    // Init the render passes
    pointPass = std::make_shared<Eng::ColorPass>(pointProgram, RenderPass::BlendingMode::Additive);
    spotPass = std::make_shared<Eng::ColorPass>(spotProgram, RenderPass::BlendingMode::Additive);
    dirPass = std::make_shared<Eng::ColorPass>(dirProgram, RenderPass::BlendingMode::Additive);
    shadowPass = std::make_shared<Eng::ShadowPass>(shadowMapProgram, shadowMapFbo);

    // Base class init method
    Eng::RenderPass::init();
}


void Eng::LightPass::configRender() {

}

void Eng::LightPass::perElementConfig(const std::shared_ptr<Eng::ListElement>& element) {
    auto light = element->getNode();
    std::shared_ptr<ColorPass> currentPass;

    if (std::dynamic_pointer_cast<Eng::SpotLight>(light)) {
        currentPass = spotPass;
    }
    else if (std::dynamic_pointer_cast<Eng::PointLight>(light)) {
        currentPass = pointPass;
    }
    else if (auto dirLight = std::dynamic_pointer_cast<Eng::DirectionalLight>(light)) {
        
        const auto& boundingBox = renderList->getSceneBoundingBox();

        std::vector<glm::vec3> cameraFrustumCorners = renderList->getEyeFrustumCorners();

        lightSpaceMatrix = dirLight->getLightSpaceMatrix(cameraFrustumCorners, boundingBox);
        
        shadowPass->start(eyeProjectionMatrix, eyeViewMatrix, lightSpaceMatrix);
        renderList->iterateAndRender(shadowPass, shadowContext);
        shadowPass->stop();

        // Activate the correct texture unit based on the shader manager parameters
        glActiveTexture(GL_TEXTURE0 + ShaderManager::SHADOW_MAP_UNIT);
        // Bind the texture to the current OpenGL context in the given unit.
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

        currentPass = dirPass;
    }
    else {
        std::cerr << "ERROR: Unsupported light type" << std::endl;
        return;
    }

    currentPass->start(eyeProjectionMatrix, eyeViewMatrix);
    renderList->iterateAndRender(currentPass, opaqueContext);
    renderList->iterateAndRender(currentPass, transparentContext);
    currentPass->stop();
}