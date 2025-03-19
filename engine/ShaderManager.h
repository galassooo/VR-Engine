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

	void setProjectionMatrix(const glm::mat4& matrix);
	void setModelViewMatrix(const glm::mat4& matrix);
	void setNormalMatrix(const glm::mat3& matrix);

	void setMaterialEmission(const glm::vec3& emission);
	void setMaterialAmbient(const glm::vec3& ambient);
	void setMaterialDiffuse(const glm::vec3& diffuse);
	void setMaterialSpecular(const glm::vec3& spec);
	void setMaterialShininess(float shininess);

	void setLightPosition(const glm::vec3& pos);
	void setLightAmbient(const glm::vec3& amb);
	void setLightDiffuse(const glm::vec3& diff);
	void setLightSpecular(const glm::vec3& spec);

private:
	/** @brief Private constructor to enforce singleton pattern */
	ShaderManager() = default;
	///< Initialization state flag
	bool initialized = false;
	bool setDefaultShaders();
	Eng::Program program;

	int projectionLocation;
	int modelViewLocation;
	int normalMatrixLocation;

	int matEmissionLoc;
	int matAmbientLoc;
	int matDiffuseLoc;
	int matSpecularLoc;
	int matShininessLoc;

	int lightPosLoc;
	int lightAmbientLoc;
	int lightDiffuseLoc;
	int lightSpecularLoc;
};