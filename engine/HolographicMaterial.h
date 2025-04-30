#pragma once

/**
 * @class HolographicMaterial
 * @brief Represents a material with a holographic effect
 *
 * This material creates a holographic appearance with animated bands,
 * color shifting, and transparency effects.
 */
class ENG_API HolographicMaterial : public Eng::Material {
public:
    /**
     * @brief Constructor for HolographicMaterial.
     *
     * @param baseColor The base color of the holographic effect.
     * @param alpha Transparency value (0.0 = fully transparent, 1.0 = opaque).
     * @param bandFrequency The frequency of holographic bands (higher = more bands).
     * @param bandSpeed The speed of band animation.
     */
    HolographicMaterial(const glm::vec3& baseColor = glm::vec3(0.2f, 0.8f, 1.0f),
        float alpha = 0.7f,
        float bandFrequency = 20.0f,
        float bandSpeed = 1.0f);

    ~HolographicMaterial() override;

    /**
     * @brief Renders the holographic material.
     *
     * Applies the holographic shader and sets its uniforms.
     */
    void render() override;

    /**
     * @brief Sets the base color of the holographic effect.
     * @param color The new base color.
     */
    void setBaseColor(const glm::vec3& color);

    /**
     * @brief Gets the base color of the holographic effect.
     * @return glm::vec3 The base color.
     */
    glm::vec3 getBaseColor() const;

    /**
     * @brief Sets the band frequency.
     * @param frequency The new frequency value.
     */
    void setBandFrequency(float frequency);

    /**
     * @brief Gets the band frequency.
     * @return float The band frequency.
     */
    float getBandFrequency() const;

    /**
     * @brief Sets the band animation speed.
     * @param speed The new speed value.
     */
    void setBandSpeed(float speed);

    /**
     * @brief Gets the band animation speed.
     * @return float The band speed.
     */
    float getBandSpeed() const;

    /**
     * @brief Sets the secondary color for the holographic effect.
     * @param color The new secondary color.
     */
    void setSecondaryColor(const glm::vec3& color);

    /**
     * @brief Gets the secondary color.
     * @return glm::vec3 The secondary color.
     */
    glm::vec3 getSecondaryColor() const;

    /**
     * @brief Initializes the holographic shader if needed.
     * @return true if successful, false otherwise.
     */
    static bool initShader();

private:
    glm::vec3 baseColor;
    glm::vec3 secondaryColor;
    float bandFrequency;
    float bandSpeed;
    float startTime;

    static std::shared_ptr<Eng::Program> holographicShader;
};