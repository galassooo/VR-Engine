#pragma once

/**
 * @class FragmentShader
 * @brief Represents a GPU fragment shader, which processes rasterized fragments to determine their final colors.
 *
 * FragmentShader compiles and encapsulates fragment-stage GLSL code, used for per-pixel operations such as
 * lighting, texturing, and color blending. Inherits base loading, compilation, and rendering interface from Shader.
 */
class ENG_API FragmentShader : public Eng::Shader {
protected:
	unsigned int create() override;
};
