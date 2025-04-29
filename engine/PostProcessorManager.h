#pragma once

/**
 * @class PostProcessorManager
 * @brief Manages a collection of post-processing effects.
 *
 * This class serves as a central manager for all post-processing effects in the engine.
 * It handles initialization, execution, and cleanup of post-processors.
 */
class ENG_API PostProcessorManager {
public:
    /**
     * @brief Gets the singleton instance of the PostProcessorManager.
     * @return Reference to the PostProcessorManager instance.
     */
    static PostProcessorManager& getInstance();

    PostProcessorManager(const PostProcessorManager&) = delete;
    PostProcessorManager& operator=(const PostProcessorManager&) = delete;

    /**
     * @brief Initializes all registered post-processors with the specified resolution.
     * @param width Width of the render target.
     * @param height Height of the render target.
     * @return True if all post-processors were initialized successfully, false otherwise.
     */
    bool initializeAll(int width, int height);

    /**
     * @brief Adds a post-processor to the manager.
     * @param postProcessor Shared pointer to the post-processor to add.
     * @return True if the post-processor was added successfully, false otherwise.
     */
    bool addPostProcessor(std::shared_ptr<PostProcessor> postProcessor);

    /**
     * @brief Removes a post-processor from the manager by name.
     * @param name Name of the post-processor to remove.
     * @return True if the post-processor was removed successfully, false otherwise.
     */
    bool removePostProcessor(const std::string& name);

    /**
     * @brief Gets a post-processor by name.
     * @param name Name of the post-processor to retrieve.
     * @return Shared pointer to the post-processor, or nullptr if not found.
     */
    std::shared_ptr<PostProcessor> getPostProcessor(const std::string& name);

    /**
     * @brief Applies all post-processors in sequence to the input texture.
     * @param inputTexture ID of the input texture.
     * @param outputTexture ID of the output texture.
     * @param width Width of the textures.
     * @param height Height of the textures.
     */
    void applyPostProcessing(unsigned int inputTexture, unsigned int outputTexture, int width, int height);

    /**
     * @brief Checks if post-processing is enabled.
     * @return True if post-processing is enabled, false otherwise.
     */
    bool isPostProcessingEnabled() const;

    /**
     * @brief Enables or disables post-processing.
     * @param enabled True to enable post-processing, false to disable.
     */
    void setPostProcessingEnabled(bool enabled);

    /**
     * @brief Gets the number of registered post-processors.
     * @return The number of post-processors.
     */
    size_t getPostProcessorCount() const;

private:
    PostProcessorManager() = default;
    ~PostProcessorManager() = default;

    /** List of registered post-processors */
    std::vector<std::shared_ptr<PostProcessor>> postProcessors;

    /** Temporary texture for intermediate processing results */
    unsigned int tempTexture = 0;

    /** Flag to enable/disable post-processing */
    bool postProcessingEnabled = true;

    /** Width of the current render targets */
    int currentWidth = 0;

    /** Height of the current render targets */
    int currentHeight = 0;

    /**
     * @brief Ensures that the temporary texture exists and has the correct size.
     * @param width Width of the texture.
     * @param height Height of the texture.
     * @return True if the texture is valid, false otherwise.
     */
    bool ensureTempTexture(int width, int height);
};