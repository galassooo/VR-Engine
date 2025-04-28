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
	if (projectionLocation == -1) {
		//std::cerr << "[ERROR]ShaderManager: projection location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setMatrix(projectionLocation, matrix);
}

void ENG_API Eng::ShaderManager::setModelViewMatrix(const glm::mat4& matrix)
{
	if (modelViewLocation == -1) {
		//std::cerr << "[ERROR]ShaderManager: model view location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setMatrix(modelViewLocation, matrix);
}

void ENG_API Eng::ShaderManager::setModelMatrix(const glm::mat4& matrix)
{
	if (modelLocation == -1) {
		//std::cerr << "[ERROR]ShaderManager: model location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setMatrix(modelLocation, matrix);
}

void ENG_API Eng::ShaderManager::setViewMatrix(const glm::mat4& matrix)
{
	if (viewLocation == -1) {
		//std::cerr << "[ERROR]ShaderManager: view location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setMatrix(viewLocation, matrix);
}

void ENG_API Eng::ShaderManager::setNormalMatrix(const glm::mat3& matrix)
{
	if (normalMatrixLocation == -1) {
		//std::cerr << "[ERROR]ShaderManager: normal matrix location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setMatrix(normalMatrixLocation, matrix);
}

void ENG_API Eng::ShaderManager::setLightSpaceMatrix(const glm::mat4& matrix)
{
	if (lightSpaceMatrixLocation == -1) {
		//std::cerr << "[ERROR]ShaderManager: light space matrix location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setMatrix(lightSpaceMatrixLocation, matrix);
}

// MATERIAL
void ENG_API Eng::ShaderManager::setMaterialEmission(const glm::vec3& emission) {
	if (matEmissionLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: material emission location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(matEmissionLoc, emission);
}
void ENG_API Eng::ShaderManager::setMaterialAmbient(const glm::vec3& ambient) {
	if (matAmbientLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: material ambient location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(matAmbientLoc, ambient);
}
void ENG_API Eng::ShaderManager::setMaterialDiffuse(const glm::vec3& diffuse) {
	if (matDiffuseLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: material diffuse location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(matDiffuseLoc, diffuse);
}
void ENG_API Eng::ShaderManager::setMaterialSpecular(const glm::vec3& spec) {
	if (matSpecularLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: material specular location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(matSpecularLoc, spec);
}
void ENG_API Eng::ShaderManager::setMaterialShininess(float shininess) {
	if (matShininessLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: material shininess location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setFloat(matShininessLoc, shininess);
}

// LIGHT
void ENG_API Eng::ShaderManager::setLightPosition(const glm::vec3& pos) {
	if (lightPosLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light position location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(lightPosLoc, pos);
}
void ENG_API Eng::ShaderManager::setLightDirection(const glm::vec3& dir) {
	if (lightDirLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light direction location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(lightDirLoc, dir);
}
void ENG_API Eng::ShaderManager::setLightCutoffAngle(float angle) {
	if (lightCutoffAngleLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light cutoff angle location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setFloat(lightCutoffAngleLoc, angle);
}
void ENG_API Eng::ShaderManager::setLightFalloff(float falloff) {
	if (lightFalloffLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light falloff location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setFloat(lightFalloffLoc, falloff);
}
void ENG_API Eng::ShaderManager::setLightAmbient(const glm::vec3& amb) {
	if (lightAmbientLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light ambient location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(lightAmbientLoc, amb);
}
void ENG_API Eng::ShaderManager::setLightDiffuse(const glm::vec3& diff) {
	if (lightDiffuseLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light diffuse location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(lightDiffuseLoc, diff);
}
void ENG_API Eng::ShaderManager::setLightSpecular(const glm::vec3& spec) {
	if (lightSpecularLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light specular location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(lightSpecularLoc, spec);
}
void ENG_API Eng::ShaderManager::setLightCastsShadows(bool castsShadows) {
	if (lightCastsShadowsLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light casts shadow location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setInt(lightCastsShadowsLoc, castsShadows ? 1 : 0);
}
void ENG_API Eng::ShaderManager::setLightAttenuation(float constant, float linear, float quadratic) {
	if (attenuationConstantLoc == -1 || attenuationLinearLoc == -1 || attenuationQuadraticLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: light attenuation location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setFloat(attenuationConstantLoc, constant);
	currentProgram->setFloat(attenuationLinearLoc, linear);
	currentProgram->setFloat(attenuationQuadraticLoc, quadratic);
}

//TEXTURES
void ENG_API Eng::ShaderManager::setUseTexture(bool use) {
	if (useTextureLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: texture use location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setInt(useTextureLoc, use ? 1 : 0);
}

/* Unused: Texture unit is defined during texture binding
void ENG_API Eng::ShaderManager::setTextureSampler(int textureUnit) {
	if (texSamplerLoc == -1)
		return;
	program.setInt(texSamplerLoc, textureUnit);
}
*/

void ENG_API Eng::ShaderManager::setGlobalLightColor(const glm::vec3& color) {
	if (globalLightColorLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: global light color location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(globalLightColorLoc, color);
}

void ENG_API Eng::ShaderManager::setEyeFront(const glm::vec3& front) {
	if (eyeFrontLoc == -1) {
		//std::cerr << "[ERROR]ShaderManager: eye front location not found in Program " << currentProgram->getGlId() << std::endl;
		return;
	}
	currentProgram->setVec3(eyeFrontLoc, front);
}

bool ENG_API Eng::ShaderManager::setDefaultShaders() {
	// Nel metodo setDefaultShaders() in ShaderManager.cpp
	const char* vs = R"(
   #version 440 core

   // Uniforms
   uniform mat4 projection;
   uniform mat4 modelview;

   // Attributes
   layout(location = 0) in vec3 in_Position;


   void main(void)
   {
      gl_Position = projection * modelview * vec4(in_Position, 1.0);
   }
)";

	// Nel metodo setDefaultShaders() in ShaderManager.cpp
	const char* fs = R"(
   #version 440 core

   out vec4 fragOutput; // Final color to render

   void main(void)
   {
      vec3 color = vec3(1.0, 0.0, 0.0);
      fragOutput = vec4(color, 1.0);
   }
)";

	// Compile vertex shader:
	std::shared_ptr<Eng::VertexShader> vertexShader = std::make_shared<Eng::VertexShader>();
	vertexShader->load(vs);

	// Compile fragment shader:
	std::shared_ptr<Eng::FragmentShader> fragmentShader = std::make_shared<Eng::FragmentShader>();
	fragmentShader->load(fs);

	// Setup shader program:
	defaultProgram = std::make_shared<Eng::Program>();

	// Important: position = 0, normal = 1, texture = 2
	defaultProgram->bindAttribute(POSITION_LOCATION, "in_Position");

	if (!defaultProgram->addShader(fragmentShader).addShader(vertexShader).build())
		return false;

	return loadProgram(defaultProgram);
}

bool ENG_API Eng::ShaderManager::loadProgram(std::shared_ptr<Eng::Program>& program) {
	if (!program || !program->getGlId())
		return false;

	//Skip loading if the same program is currently in use
	if (currentProgram == program) {
		//std::cout << "[DEBUG]ShaderManager: Skipping loading, Program " << program->getGlId() << " already loaded" << std::endl;
		return true;
	}

	projectionLocation = program->getParamLocation(UNIFORM_PROJECTION_MATRIX);
	modelViewLocation = program->getParamLocation(UNIFORM_MODELVIEW_MATRIX);
	modelLocation = program->getParamLocation(UNIFORM_MODEL_MATRIX);
	viewLocation = program->getParamLocation(UNIFORM_VIEW_MATRIX);
	normalMatrixLocation = program->getParamLocation(UNIFORM_NORMAL_MATRIX);
	lightSpaceMatrixLocation = program->getParamLocation(UNIFORM_LIGHTSPACE_MATRIX);

	matEmissionLoc = program->getParamLocation(UNIFORM_MATERIAL_EMISSION);
	matAmbientLoc = program->getParamLocation(UNIFORM_MATERIAL_AMBIENT);
	matDiffuseLoc = program->getParamLocation(UNIFORM_MATERIAL_DIFFUSE);
	matSpecularLoc = program->getParamLocation(UNIFORM_MATERIAL_SPECULAR);
	matShininessLoc = program->getParamLocation(UNIFORM_MATERIAL_SHININESS);

	lightPosLoc = program->getParamLocation(UNIFORM_LIGHT_POSITION);
	lightDirLoc = program->getParamLocation(UNIFORM_LIGHT_DIRECTION);	//Aggiunto per luci direzionali
	lightCutoffAngleLoc = program->getParamLocation(UNIFORM_LIGHT_CUTOFF_ANGLE);	//Aggiunto per spotlights
	lightFalloffLoc = program->getParamLocation(UNIFORM_LIGHT_FALLOFF);	//Aggiunto per spotlights
	lightAmbientLoc = program->getParamLocation(UNIFORM_LIGHT_AMBIENT);
	lightDiffuseLoc = program->getParamLocation(UNIFORM_LIGHT_DIFFUSE);
	lightSpecularLoc = program->getParamLocation(UNIFORM_LIGHT_SPECULAR);
	lightCastsShadowsLoc = program->getParamLocation(UNIFORM_LIGHT_CASTS_SHADOWS);	//Aggiunto per shodow mapping nelle luci direzionali (WIP)
	attenuationConstantLoc = program->getParamLocation(UNIFORM_ATTENUATION_CONSTANT);
	attenuationLinearLoc = program->getParamLocation(UNIFORM_ATTENUATION_LINEAR);
	attenuationQuadraticLoc = program->getParamLocation(UNIFORM_ATTENUATION_QUADRATIC);

	//texSamplerLoc = program->getParamLocation(UNIFORM_TEXTURE_DIFFUSE); //not used: handled engine side when binding the texture
	useTextureLoc = program->getParamLocation(UNIFORM_USE_TEXTURE_DIFFUSE);

	globalLightColorLoc = program->getParamLocation(UNIFORM_GLOBAL_LIGHT_COLOR);

	eyeFrontLoc = program->getParamLocation(UNIFORM_EYE_FRONT);

	program->render();
	currentProgram = program;

	return true;
}

std::string ENG_API Eng::ShaderManager::preprocessShaderCode(const std::string& source) {
	auto symbolMap = buildShaderSymbolMap();

	std::string result = source;

	// Sostituzione automatica dei simboli con i valori delle costanti
	for (const auto& entry : symbolMap) {
		size_t pos = 0;
		const std::string& replacement = entry.second;

		// Sostituzione della stringa
		while ((pos = result.find(entry.first, pos)) != std::string::npos) {
			result.replace(pos, entry.first.length(), replacement);
			pos += replacement.length();
		}
	}

	return result;
}

std::unordered_map<std::string, std::string> ENG_API Eng::ShaderManager::buildShaderSymbolMap() {
	return {
		{"ShaderManager::POSITION_LOCATION", std::to_string(POSITION_LOCATION)},
		{"ShaderManager::NORMAL_LOCATION", std::to_string(NORMAL_LOCATION)},
		{"ShaderManager::TEX_COORD_LOCATION", std::to_string(TEX_COORD_LOCATION)},
		{"ShaderManager::DIFFUSE_TEXTURE_UNIT", std::to_string(DIFFUSE_TEXTURE_UNIT)},
		{"ShaderManager::SHADOW_MAP_UNIT", std::to_string(SHADOW_MAP_UNIT)},

		{"ShaderManager::UNIFORM_PROJECTION_MATRIX", UNIFORM_PROJECTION_MATRIX},
		{"ShaderManager::UNIFORM_MODELVIEW_MATRIX", UNIFORM_MODELVIEW_MATRIX},
		{"ShaderManager::UNIFORM_MODEL_MATRIX", UNIFORM_MODEL_MATRIX},
		{"ShaderManager::UNIFORM_VIEW_MATRIX", UNIFORM_VIEW_MATRIX},
		{"ShaderManager::UNIFORM_NORMAL_MATRIX", UNIFORM_NORMAL_MATRIX},
		{"ShaderManager::UNIFORM_LIGHTSPACE_MATRIX", UNIFORM_LIGHTSPACE_MATRIX},
		{"ShaderManager::UNIFORM_MATERIAL_EMISSION", UNIFORM_MATERIAL_EMISSION},
		{"ShaderManager::UNIFORM_MATERIAL_AMBIENT", UNIFORM_MATERIAL_AMBIENT},
		{"ShaderManager::UNIFORM_MATERIAL_DIFFUSE", UNIFORM_MATERIAL_DIFFUSE},
		{"ShaderManager::UNIFORM_MATERIAL_SPECULAR", UNIFORM_MATERIAL_SPECULAR},
		{"ShaderManager::UNIFORM_MATERIAL_SHININESS", UNIFORM_MATERIAL_SHININESS},
		{"ShaderManager::UNIFORM_USE_TEXTURE_DIFFUSE", UNIFORM_USE_TEXTURE_DIFFUSE},
		{"ShaderManager::UNIFORM_LIGHT_POSITION", UNIFORM_LIGHT_POSITION},
		{"ShaderManager::UNIFORM_LIGHT_DIRECTION", UNIFORM_LIGHT_DIRECTION},
		{"ShaderManager::UNIFORM_LIGHT_AMBIENT", UNIFORM_LIGHT_AMBIENT},
		{"ShaderManager::UNIFORM_LIGHT_DIFFUSE", UNIFORM_LIGHT_DIFFUSE},
		{"ShaderManager::UNIFORM_LIGHT_SPECULAR", UNIFORM_LIGHT_SPECULAR},
		{"ShaderManager::UNIFORM_LIGHT_CASTS_SHADOWS", UNIFORM_LIGHT_CASTS_SHADOWS},
		{"ShaderManager::UNIFORM_ATTENUATION_CONSTANT", UNIFORM_ATTENUATION_CONSTANT},
		{"ShaderManager::UNIFORM_ATTENUATION_LINEAR", UNIFORM_ATTENUATION_LINEAR},
		{"ShaderManager::UNIFORM_ATTENUATION_QUADRATIC", UNIFORM_ATTENUATION_QUADRATIC},
		{"ShaderManager::UNIFORM_LIGHT_CUTOFF_ANGLE", UNIFORM_LIGHT_CUTOFF_ANGLE},
		{"ShaderManager::UNIFORM_LIGHT_FALLOFF", UNIFORM_LIGHT_FALLOFF},
		{"ShaderManager::UNIFORM_GLOBAL_LIGHT_COLOR", UNIFORM_GLOBAL_LIGHT_COLOR},
		{"ShaderManager::UNIFORM_EYE_FRONT", UNIFORM_EYE_FRONT}
	};
}
