#include "engine.h"

// GLEW
#include <GL/glew.h>

/**
 * @brief Initializes the shaders used during rendering.
 *
 * This method loads and compiles the vertex and fragment shaders for basic rendering,
 * shadow mapping, and light contributions. It also sets up the shader programs for use.
 *
 * @return true if shaders are initialized successfully, false otherwise.
 */
bool Eng::MultipassRenderer::init() {
	if (initialized) {
		return true; 
	}

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


    //Compile and link Basic Shaders used for the first pass
    baseColorProgram = std::make_shared<Eng::Program>();
    baseColorProgram->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position").bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal").bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");
    baseColorProgram->bindSampler(ShaderManager::DIFFUSE_TEXTURE_UNIT, "texSampler");
    if (!baseColorProgram->addShader(basicFragmentShader).addShader(basicVertexShader).build())
        return false;

    // Init Render Passes
    baseColorPass = std::make_shared<Eng::ColorPass>(baseColorProgram, RenderPass::BlendingMode::Standard);
    lightingPass = std::make_shared<Eng::LightPass>(basicVertexShader);

    initialized = true;
    return initialized;
}

void Eng::MultipassRenderer::runPass(const std::shared_ptr<Eng::RenderPass>& pass, const std::shared_ptr<Eng::RenderPassContext>& context) {
    pass->start(eyeProjectionMatrix, eyeViewMatrix);
    renderList->iterateAndRender(pass, context);
    pass->stop();
}

void Eng::MultipassRenderer::render(const std::shared_ptr<Eng::List>& renderList) {
    glEnable(GL_DEPTH_TEST);

    this->renderList = renderList;
    lightingPass->setRenderList(this->renderList);

    eyeViewMatrix = renderList->getEyeViewMatrix();
    eyeProjectionMatrix = renderList->getEyeProjectionMatrix();

    baseColorPass->setGlobalLightColor(globalLightColor);

    runPass(baseColorPass, meshContext);

    runPass(lightingPass, lightContext);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}