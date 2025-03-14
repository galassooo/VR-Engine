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

private:
	/** @brief Private constructor to enforce singleton pattern */
	ShaderManager() = default;
	///< Initialization state flag
	bool initialized = false;
	bool setDefaultShaders();
	Eng::Program program;
	int projectionLocation;
	int modelViewLocation;
};