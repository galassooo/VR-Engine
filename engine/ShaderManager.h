#pragma once

/**
 * @class ShaderManager
 * @brief Manages Shaders to be built into the GPU Program for the graphics engine
 *
 * The ShaderManager implements the Singleton pattern to provide centralized management
 * of all shaders and programs. It handles:
 * - Registration and binding of default shaders
 * - Management of custom shaders that can be dynamically added/removed from the rendering cycle
 */
class ENG_API ShaderManager {
public:
	static ShaderManager& getInstance();
	bool initialize();
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;

	// LOCATIONS and UNITS
	static constexpr int POSITION_LOCATION = 0;		//Location bound to position coordinates in the Vertex Shader
	static constexpr int NORMAL_LOCATION = 1;		//Location bound to normal coordinates in the Vertex Shader
	static constexpr int TEX_COORD_LOCATION = 2;	//Location bound to texture coordinates in the Vertex Shader
	static constexpr int DIFFUSE_TEXTURE_UNIT = 0;	//Texture Unit bound to the diffuse texture sampler in the Fragment Shader
	static constexpr int SHADOW_MAP_UNIT = 1;		//Texture Unit bound to the shadow map sampler in the Fragment Shader

	// VARIABLE NAMES
	static constexpr const char* UNIFORM_PROJECTION_MATRIX = "projection";		//Projection matrix - Uniform name
	static constexpr const char* UNIFORM_MODEL_MATRIX = "model";				//Model matrix - Uniform name
	static constexpr const char* UNIFORM_VIEW_MATRIX = "view";					//View matrix - Uniform name
	static constexpr const char* UNIFORM_MODELVIEW_MATRIX = "modelview";		//ModelView matrix - Uniform name
	static constexpr const char* UNIFORM_NORMAL_MATRIX = "normalMatrix";		//Normal matrix - Uniform name
	static constexpr const char* UNIFORM_LIGHTSPACE_MATRIX = "lightspaceMatrix";//Light Space matrix - Uniform name

	static constexpr const char* UNIFORM_MATERIAL_EMISSION = "matEmission";		//Material emission - Uniform name
	static constexpr const char* UNIFORM_MATERIAL_AMBIENT = "matAmbient";		//Material ambient contribution - Uniform name
	static constexpr const char* UNIFORM_MATERIAL_DIFFUSE = "matDiffuse";		//Material diffuse contribution - Uniform name
	static constexpr const char* UNIFORM_MATERIAL_SPECULAR = "matSpecular";		//Material specular contribution - Uniform name
	static constexpr const char* UNIFORM_MATERIAL_SHININESS = "matShininess";	//Material shininess - Uniform name
	static constexpr const char* UNIFORM_USE_TEXTURE_DIFFUSE = "useTexture";	//Diffuse texture use flag (bool) - Uniform name
	//static constexpr const char* UNIFORM_TEXTURE_DIFFUSE = "texSampler";		//(Unused)Diffuse texture sampler - Uniform name

	static constexpr const char* UNIFORM_LIGHT_POSITION = "lightPos";						//Light position - Uniform name
	static constexpr const char* UNIFORM_LIGHT_DIRECTION = "lightDir";						//Light direction - Uniform name
	static constexpr const char* UNIFORM_LIGHT_CUTOFF_ANGLE = "lightCutoff";				//Spot Light cutoff angle - Uniform name
	static constexpr const char* UNIFORM_LIGHT_FALLOFF = "lightFalloff";					//Spot Light falloff - Uniform name
	static constexpr const char* UNIFORM_LIGHT_AMBIENT = "lightAmbient";					//Light ambient contribution - Uniform name
	static constexpr const char* UNIFORM_LIGHT_DIFFUSE = "lightDiffuse";					//Light diffuse contribution - Uniform name
	static constexpr const char* UNIFORM_LIGHT_SPECULAR = "lightSpecular";					//Light specular contribution - Uniform name
	static constexpr const char* UNIFORM_LIGHT_CASTS_SHADOWS = "useShadowMap";				//Light shadow cast flag (bool) - Uniform name
	static constexpr const char* UNIFORM_ATTENUATION_CONSTANT = "constAttenuatuion";		//Light attenuation constant - Uniform name
	static constexpr const char* UNIFORM_ATTENUATION_LINEAR = "linearAttenuation";			//Light attenuation linear - Uniform name
	static constexpr const char* UNIFORM_ATTENUATION_QUADRATIC = "quadraticAttenuation";	//Light attenuation quadratic - Uniform name
	//static constexpr const char* UNIFORM_TEXTURE_SHADOWS = "shadowMap";					//(Unused)Shadow map texture sampler - Uniform name

	static constexpr const char* UNIFORM_GLOBAL_LIGHT_COLOR = "globalLightColor";		//Global light color - Uniform name

	static constexpr const char* UNIFORM_EYE_FRONT = "eyeFront";	//Camera front vector - Uniform name


	bool loadProgram(std::shared_ptr<Eng::Program>& program);

	void setProjectionMatrix(const glm::mat4& matrix);
	void setModelViewMatrix(const glm::mat4& matrix);
	void setModelMatrix(const glm::mat4& matrix);
	void setViewMatrix(const glm::mat4& matrix);
	void setNormalMatrix(const glm::mat3& matrix);
	void setLightSpaceMatrix(const glm::mat4& matrix);

	void setMaterialEmission(const glm::vec3& emission);
	void setMaterialAmbient(const glm::vec3& ambient);
	void setMaterialDiffuse(const glm::vec3& diffuse);
	void setMaterialSpecular(const glm::vec3& spec);
	void setMaterialShininess(float shininess);

	void setLightPosition(const glm::vec3& pos);
	void setLightDirection(const glm::vec3& pos);
	void setLightCutoffAngle(float angle);
	void setLightFalloff(float falloff);
	void setLightAmbient(const glm::vec3& amb);
	void setLightDiffuse(const glm::vec3& diff);
	void setLightSpecular(const glm::vec3& spec);
	void setLightCastsShadows(bool castsShadows);
	void setLightAttenuation(float constant, float linear, float quadratic);

	void setUseTexture(bool use);
	//void setTextureSampler(int textureUnit); unused: texture unit is defined during texture binding

	void setGlobalLightColor(const glm::vec3& color);

	void setEyeFront(const glm::vec3& front);


	const glm::mat4& getCachedProjectionMatrix()  const { return cachedProjection; }
	const glm::mat4& getCachedModelViewMatrix()   const { return cachedModelView; }
	const glm::mat3& getCachedNormalMatrix()      const { return cachedNormal; }
	const glm::mat4& getCachedLightSpaceMatrix()  const { return cachedLightSpace; }
	const glm::vec3& getCachedEyeFront()          const { return cachedEyeFront; }
	const glm::vec3& getCachedGlobalLightColor()  const { return cachedGlobalLight; }


	static std::string preprocessShaderCode(const std::string& source);
	std::shared_ptr<Eng::Program> getCurrentProgram() const { return currentProgram; }

private:
	/** @brief Private constructor to enforce singleton pattern */
	ShaderManager() = default;
	///< Initialization state flag
	bool initialized = false;
	bool setDefaultShaders();

	static std::unordered_map<std::string, std::string> buildShaderSymbolMap();

	std::shared_ptr<Eng::Program> defaultProgram;
	std::shared_ptr<Eng::Program> currentProgram;

	//int texSamplerLoc; not necessary, texture sampler location is set engine side when binding the texture
	int useTextureLoc;

	int projectionLocation;
	int modelViewLocation;
	int modelLocation;
	int viewLocation;
	int normalMatrixLocation;
	int lightSpaceMatrixLocation;

	int matEmissionLoc;
	int matAmbientLoc;
	int matDiffuseLoc;
	int matSpecularLoc;
	int matShininessLoc;

	int lightPosLoc;
	int lightDirLoc;
	int lightCutoffAngleLoc;
	int lightFalloffLoc;
	int lightAmbientLoc;
	int lightDiffuseLoc;
	int lightSpecularLoc;
	int lightCastsShadowsLoc;
	int attenuationConstantLoc;
	int attenuationLinearLoc;
	int attenuationQuadraticLoc;

	int globalLightColorLoc;

	int eyeFrontLoc;

	// cache degli ultimi valori inviati agli uniform comuni
	glm::mat4 cachedProjection = glm::mat4(1.0f);
	glm::mat4 cachedModelView = glm::mat4(1.0f);
	glm::mat3 cachedNormal = glm::mat3(1.0f);
	glm::mat4 cachedLightSpace = glm::mat4(1.0f);
	glm::vec3 cachedEyeFront = glm::vec3(0.0f);
	glm::vec3 cachedGlobalLight = glm::vec3(1.0f);

};