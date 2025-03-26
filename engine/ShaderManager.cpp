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
		std::cerr << "ShaderManager::initialize() already called. Skipping reinitialization." << std::endl;
		return false;
	}

	if (!setDefaultShaders())
		return false;

	glUseProgram(program.getGlId());

	initialized = true;
	return true;
}

// MATRICES
void ENG_API Eng::ShaderManager::setProjectionMatrix(const glm::mat4& matrix)
{
	program.setMatrix(projectionLocation, matrix);
}

void ENG_API Eng::ShaderManager::setModelViewMatrix(const glm::mat4& matrix)
{
	program.setMatrix(modelViewLocation, matrix);
}

void ENG_API Eng::ShaderManager::setNormalMatrix(const glm::mat3& matrix)
{
	program.setMatrix(normalMatrixLocation, matrix);
}

// MATERIAL
void ENG_API Eng::ShaderManager::setMaterialEmission(const glm::vec3& emission) {
	program.setVec3(matEmissionLoc, emission);
}
void ENG_API Eng::ShaderManager::setMaterialAmbient(const glm::vec3& ambient) {
	program.setVec3(matAmbientLoc, ambient);
}
void ENG_API Eng::ShaderManager::setMaterialDiffuse(const glm::vec3& diffuse) {
	program.setVec3(matDiffuseLoc, diffuse);
}
void ENG_API Eng::ShaderManager::setMaterialSpecular(const glm::vec3& spec) {
	program.setVec3(matSpecularLoc, spec);
}
void ENG_API Eng::ShaderManager::setMaterialShininess(float shininess) {
	program.setFloat(matShininessLoc, shininess);
}

// LIGHT
void ENG_API Eng::ShaderManager::setLightPosition(const glm::vec3& pos) {
	program.setVec3(lightPosLoc, pos);
}
void ENG_API Eng::ShaderManager::setLightAmbient(const glm::vec3& amb) {
	program.setVec3(lightAmbientLoc, amb);
}
void ENG_API Eng::ShaderManager::setLightDiffuse(const glm::vec3& diff) {
	program.setVec3(lightDiffuseLoc, diff);
}
void ENG_API Eng::ShaderManager::setLightSpecular(const glm::vec3& spec) {
	program.setVec3(lightSpecularLoc, spec);
}

//TEXTURES
void ENG_API Eng::ShaderManager::setUseTexture(bool use) {
	program.setInt(useTextureLoc, use ? 1 : 0);
}

void ENG_API Eng::ShaderManager::setTextureSampler(int textureUnit) {
	program.setInt(texSamplerLoc, textureUnit);
}

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
	program = Eng::Program();

	// Important: position = 0, normal = 1
	program.bind(0, "in_Position");
	program.bind(1, "in_Normal");

	if (!program.addShader(fragmentShader).addShader(vertexShader).build())
		return false;

	projectionLocation = program.getParamLocation("projection");
	modelViewLocation = program.getParamLocation("modelview");
	normalMatrixLocation = program.getParamLocation("normalMatrix");

	matEmissionLoc = program.getParamLocation("matEmission");
	matAmbientLoc = program.getParamLocation("matAmbient");
	matDiffuseLoc = program.getParamLocation("matDiffuse");
	matSpecularLoc = program.getParamLocation("matSpecular");
	matShininessLoc = program.getParamLocation("matShininess");

	lightPosLoc = program.getParamLocation("lightPos");
	lightAmbientLoc = program.getParamLocation("lightAmbient");
	lightDiffuseLoc = program.getParamLocation("lightDiffuse");
	lightSpecularLoc = program.getParamLocation("lightSpecular");

	texSamplerLoc = program.getParamLocation("texSampler");
	useTextureLoc = program.getParamLocation("useTexture");

	return true;
}
