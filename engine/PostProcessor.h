#pragma once

/**
 * @class PostProcessor
 * @brief Abstract base class for post-processing effects.
 *
 * This class defines the interface for all post-processing effects in the engine.
 * Derived classes should implement the specific post-processing algorithms.
 */
class ENG_API PostProcessor {
public:
    PostProcessor() = default;
    virtual ~PostProcessor() = default;

    /**
     * @brief Initializes the post-processor with the specified dimensions.
     * @param width Width of the effect's render target.
     * @param height Height of the effect's render target.
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool init(int width, int height) = 0;

    /**
     * @brief Checks if the post-processor has been initialized.
     * @return true if initialized, false otherwise.
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Applies the post-processing effect to the input texture and writes to the output texture.
     * @param inputTexture OpenGL texture ID containing the input image.
     * @param outputTexture OpenGL texture ID where the result will be written.
     * @param width Width of the textures.
     * @param height Height of the textures.
     */
    virtual void applyEffect(unsigned int inputTexture, unsigned int outputTexture, int width, int height) = 0;

    /**
     * @brief Sets a named parameter for the post-processor.
     *
     * This allows for generic parameter setting without having to know the specific
     * post-processor implementation. Each derived class should interpret the parameters
     * it recognizes and ignore the others.
     *
     * @param name Parameter name.
     * @param value Parameter value as a float.
     */
    virtual void setParameter(const std::string& name, float value) = 0;

    /**
     * @brief Gets the name of the post-processor.
     * @return The name of the post-processor.
     */
    virtual std::string getName() const = 0;
};