#include "engine.h"

// GLEW
#include <GL/glew.h>

ENG_API Eng::ShaderManager& Eng::ShaderManager::getInstance()
{
	static Eng::ShaderManager instance;
	return instance;
}

bool ENG_API Eng::ShaderManager::initialize() {
	if (initialized) {
		std::cout << "ShaderManager::initialize() already called. Skipping reinitialization." << std::endl;
		return true;
	}

	if (!setDefaultShaders())
		return false;

	initialized = true;
	return true;
}

// MATRICES
void ENG_API Eng::ShaderManager::setProjectionMatrix(const glm::mat4& matrix)
{
	if (projectionLocation == -1)
		return;
	currentProgram->setMatrix(projectionLocation, matrix);
}

void ENG_API Eng::ShaderManager::setModelViewMatrix(const glm::mat4& matrix)
{
	if (modelViewLocation == -1)
		return;
	currentProgram->setMatrix(modelViewLocation, matrix);
}

void ENG_API Eng::ShaderManager::setNormalMatrix(const glm::mat3& matrix)
{
	if (normalMatrixLocation == -1)
		return;
	currentProgram->setMatrix(normalMatrixLocation, matrix);
}

// MATERIAL
void ENG_API Eng::ShaderManager::setMaterialEmission(const glm::vec3& emission) {
	if (matEmissionLoc == -1)
		return;
	currentProgram->setVec3(matEmissionLoc, emission);
}
void ENG_API Eng::ShaderManager::setMaterialAmbient(const glm::vec3& ambient) {
	if (matAmbientLoc == -1)
		return;
	currentProgram->setVec3(matAmbientLoc, ambient);
}
void ENG_API Eng::ShaderManager::setMaterialDiffuse(const glm::vec3& diffuse) {
	if (matDiffuseLoc == -1)
		return;
	currentProgram->setVec3(matDiffuseLoc, diffuse);
}
void ENG_API Eng::ShaderManager::setMaterialSpecular(const glm::vec3& spec) {
	if (matSpecularLoc == -1)
		return;
	currentProgram->setVec3(matSpecularLoc, spec);
}
void ENG_API Eng::ShaderManager::setMaterialShininess(float shininess) {
	if (matShininessLoc == -1)
		return;
	currentProgram->setFloat(matShininessLoc, shininess);
}

// LIGHT
void ENG_API Eng::ShaderManager::setLightPosition(const glm::vec3& pos) {
	if (lightPosLoc == -1)
		return;
	currentProgram->setVec3(lightPosLoc, pos);
}
void ENG_API Eng::ShaderManager::setLightDirection(const glm::vec3& dir) {
	if (lightDirLoc == -1)
		return;
	currentProgram->setVec3(lightDirLoc, dir);
}
void ENG_API Eng::ShaderManager::setLightAmbient(const glm::vec3& amb) {
	if (lightAmbientLoc == -1)
		return;
	currentProgram->setVec3(lightAmbientLoc, amb);
}
void ENG_API Eng::ShaderManager::setLightDiffuse(const glm::vec3& diff) {
	if (lightDiffuseLoc == -1)
		return;
	currentProgram->setVec3(lightDiffuseLoc, diff);
}
void ENG_API Eng::ShaderManager::setLightSpecular(const glm::vec3& spec) {
	if (lightSpecularLoc == -1)
		return;
	currentProgram->setVec3(lightSpecularLoc, spec);
}
void ENG_API Eng::ShaderManager::setLightCastsShadows(bool castsShadows) {
	if (lightCastsShadowsLoc == -1)
		return;
	currentProgram->setInt(lightCastsShadowsLoc, castsShadows ? 1 : 0);
}

//TEXTURES
void ENG_API Eng::ShaderManager::setUseTexture(bool use) {
	if (useTextureLoc == -1)
		return;
	currentProgram->setInt(useTextureLoc, use ? 1 : 0);
}

/* Unused: Texture unit is defined during texture binding
void ENG_API Eng::ShaderManager::setTextureSampler(int textureUnit) {
	if (texSamplerLoc == -1)
		return;
	program.setInt(texSamplerLoc, textureUnit);
}
*/

bool ENG_API Eng::ShaderManager::setDefaultShaders() {
	// Nel metodo setDefaultShaders() in ShaderManager.cpp
	const char* vs = R"(
   #version 440 core

   // Uniforms
   uniform mat4 projection;
   uniform mat4 modelview;
   uniform mat3 normalMatrix;

   // Attributes
   layout(location = 0) in vec3 in_Position;
   layout(location = 1) in vec3 in_Normal;
   layout(location = 2) in vec2 in_TexCoord;  // Aggiunto per texture

   // Varying (Passing to fragment shader):
   out vec4 fragPos;
   out vec3 fragNormal;
   out vec2 texCoord;  // Aggiunto per texture

   void main(void)
   {
      // 1) Transform the incoming vertex position to eye space:
      fragPos = modelview * vec4(in_Position, 1.0);

      // 2) Transform to clip space by applying the projection.
      gl_Position = projection * fragPos;

      // 3) Transform the normal from object space into eye space
      fragNormal = normalMatrix * in_Normal;
      
      // 4) Pass texture coordinates to fragment shader
      texCoord = in_TexCoord;
   }
)";

	// Nel metodo setDefaultShaders() in ShaderManager.cpp
	const char* fs = R"(
   #version 440 core
    
   // Varying variables from vertex shader
   in vec4 fragPos;
   in vec3 fragNormal;
   in vec2 texCoord;  // Aggiunto per texture

   out vec4 fragOutput; // Final color to render

   // Material properties:
   uniform vec3 matEmission;
   uniform vec3 matAmbient;
   uniform vec3 matDiffuse;
   uniform vec3 matSpecular;
   uniform float matShininess;

   // Light properties
   uniform vec3 lightPos;
   uniform vec3 lightAmbient;
   uniform vec3 lightDiffuse;
   uniform vec3 lightSpecular;
   
   // Texture mapping:
   layout(binding = 0) uniform sampler2D texSampler;
   uniform bool useTexture;  // Flag per indicare se usare la texture

   void main(void)
   {
      // Emission + ambient
      vec3 color = matEmission + (matAmbient * lightAmbient);

      // Interpolated normal form the vertex shader
      vec3 N = normalize(fragNormal);

      // Light direction in eye-space
      vec3 L = normalize(lightPos - fragPos.xyz);

      // Lambert's cosine term
      float lambert = max(dot(N, L), 0.0);

      if (lambert > 0.0)
      {
         // Add diffuse contribution
         color += matDiffuse * lambert * lightDiffuse;

         // Blinn-Phong specular
         vec3 V = normalize(-fragPos.xyz);
         vec3 H = normalize(L + V);

         float specAngle = max(dot(N, H), 0.0);
         color += matSpecular * pow(specAngle, matShininess) * lightSpecular;
      }
      
      // Final color calculation with texture
      if (useTexture) {
         vec4 texColor = texture(texSampler, texCoord);
         fragOutput = vec4(color, 1.0) * texColor;
      } else {
         fragOutput = vec4(color, 1.0);
      }
   }
)";

	// Compile vertex shader:
	std::shared_ptr<Eng::VertexShader> vertexShader = std::make_shared<Eng::VertexShader>();
	vertexShader->load(vs);

	// Compile fragment shader:
	std::shared_ptr<Eng::FragmentShader> fragmentShader = std::make_shared<Eng::FragmentShader>();
	fragmentShader->load(fs);

	// Setup shader program:
	std::shared_ptr<Eng::Program> program = std::make_shared<Eng::Program>();

	// Important: position = 0, normal = 1, texture = 2
	program->bindAttribute(0, "in_Position").bindAttribute(1, "in_Normal").bindAttribute(2, "in_TexCoord");
	program->bindSampler(DIFFUSE_TEXURE_UNIT, "texSampler");

	if (!program->addShader(fragmentShader).addShader(vertexShader).build())
		return false;

	return loadProgram(program);
}

bool ENG_API Eng::ShaderManager::loadProgram(std::shared_ptr<Eng::Program>& program) {
	if (!program || !program->getGlId())
		return false;

	if (currentProgram == program) {
		std::cout << "[DEBUG]ShaderManager: Skipping loading, Program " << program->getGlId() << " already loaded" << std::endl;
		return true;
	}

	projectionLocation = program->getParamLocation(UNIFORM_PROJECTION_MATRIX);
	modelViewLocation = program->getParamLocation(UNIFORM_MODELVIEW_MATRIX);
	normalMatrixLocation = program->getParamLocation(UNIFORM_NORMAL_MATRIX);

	matEmissionLoc = program->getParamLocation(UNIFORM_MATERIAL_EMISSION);
	matAmbientLoc = program->getParamLocation(UNIFORM_MATERIAL_AMBIENT);
	matDiffuseLoc = program->getParamLocation(UNIFORM_MATERIAL_DIFFUSE);
	matSpecularLoc = program->getParamLocation(UNIFORM_MATERIAL_SPECULAR);
	matShininessLoc = program->getParamLocation(UNIFORM_MATERIAL_SHININESS);

	lightPosLoc = program->getParamLocation(UNIFORM_LIGHT_POSITION);
	lightDirLoc = program->getParamLocation(UNIFORM_LIGHT_DIRECTION);	//Aggiunto per luci direzionali e spot lights
	lightAmbientLoc = program->getParamLocation(UNIFORM_LIGHT_AMBIENT);
	lightDiffuseLoc = program->getParamLocation(UNIFORM_LIGHT_DIFFUSE);
	lightSpecularLoc = program->getParamLocation(UNIFORM_LIGHT_SPECULAR);
	lightCastsShadowsLoc = program->getParamLocation(UNIFORM_LIGHT_CASTS_SHADOWS);	//Aggiunto per shodow mapping nelle luci direzionali (WIP)

	//texSamplerLoc = program->getParamLocation(UNIFORM_TEXTURE_DIFFUSE); //not used: handled engine side when binding the texture
	useTextureLoc = program->getParamLocation(UNIFORM_USE_TEXTURE_DIFFUSE);

	program->render();
	currentProgram = program;

	return true;
}
