
/**
 * @file HolographicMaterial.cpp
 * @brief Implementation of a custom material with a holographic shader effect.
 */

#include "Engine.h"
#include <GL/glew.h>
#include <chrono>

// init static members
std::shared_ptr<Eng::Program> Eng::HolographicMaterial::holographicShader = nullptr;


/**
 * @brief Constructor for HolographicMaterial.
 * @param baseColor The base RGB color of the material.
 * @param alpha Maximum opacity of the material (0.0 - 1.0).
 * @param bandFrequency Frequency of horizontal scanlines (cycles per unit).
 * @param bandSpeed Speed of band movement over time.
 */
Eng::HolographicMaterial::HolographicMaterial(const glm::vec3& baseColor, float alpha,
    float bandFrequency, float bandSpeed)
    : Material(baseColor, alpha, 0.5f, glm::vec3(0.0f)),
    baseColor(baseColor),
    secondaryColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    bandFrequency(bandFrequency),
    bandSpeed(bandSpeed),
    startTime(std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count()) {

    if (!holographicShader) {
        initShader();
    }
}

/**
 * @brief Destructor for HolographicMaterial.
 */
Eng::HolographicMaterial::~HolographicMaterial() {
    // No specific cleanup required
}

/**
 * @brief Initializes and compiles the holographic shader.
 * @return True if compilation and linking succeed, false otherwise.
 */
bool Eng::HolographicMaterial::initShader() {
    const std::string vertexShaderCode = R"(
    #version 440 core
    
    // Uniforms
    uniform mat4 ShaderManager::UNIFORM_PROJECTION_MATRIX;
    uniform mat4 ShaderManager::UNIFORM_MODELVIEW_MATRIX;
    uniform mat3 ShaderManager::UNIFORM_NORMAL_MATRIX;
    
    // Attributes
    layout(location = ShaderManager::POSITION_LOCATION) in vec3 in_Position;
    layout(location = ShaderManager::NORMAL_LOCATION) in vec3 in_Normal;
    layout(location = ShaderManager::TEX_COORD_LOCATION) in vec2 in_TexCoord;
    
    // Output to fragment shader
    out vec3 fragPos;
    out vec3 fragNormal;
    out vec2 texCoord;
    out vec3 viewDirection;
    
    void main(void) {
        // Transform vertex to eye space
        vec4 positionEye = ShaderManager::UNIFORM_MODELVIEW_MATRIX * vec4(in_Position, 1.0);
        fragPos = positionEye.xyz;
        
        // Normal in eye space
        fragNormal = normalize(ShaderManager::UNIFORM_NORMAL_MATRIX * in_Normal);
        
        // View direction (from fragment to camera)
        viewDirection = normalize(-fragPos);
        
        // Pass texture coordinates
        texCoord = in_TexCoord;
        
        // Final position
        gl_Position = ShaderManager::UNIFORM_PROJECTION_MATRIX * positionEye;
    }
    )";


    const std::string fragmentShaderCode = R"(
    #version 440 core

    in vec3 fragPos;        // eye-space (x,y,z)
    in vec3 fragNormal;
    in vec3 viewDirection;

    out vec4 fragColor;

    uniform vec3  baseColor;
    uniform vec3  secondaryColor;
    uniform float alpha;
    uniform float bandFrequency;
    uniform float bandSpeed;
    uniform float time;

    const float BAND_FILL   = 0.45;
    const float BAND_EDGE   = 0.04;

    void main()
    {
        //border 
        vec3  N = normalize(fragNormal);
        vec3  V = normalize(viewDirection);
        float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);

        // band, changes value between 0 -1 depending on time
        float coord = fract(fragPos.y * bandFrequency + time * bandSpeed);

        //mask for bands
        float maskBegin = smoothstep(0.0,  BAND_EDGE,      coord);
        float maskEnd   = smoothstep(BAND_FILL,
                                     BAND_FILL + BAND_EDGE, coord);
        float bandMask  = maskBegin * (1.0 - maskEnd);

        // color and transparency
        vec3  col   = mix(baseColor, secondaryColor, bandMask);
        col        += fresnel * secondaryColor * 0.25;

        float outAlpha = alpha * bandMask;

        fragColor = vec4(col, outAlpha);
    }

    )";

    //compiles shaders
    std::shared_ptr<Eng::VertexShader> vertexShader = std::make_shared<Eng::VertexShader>();
    if (!vertexShader->load(ShaderManager::preprocessShaderCode(vertexShaderCode).c_str())) {
        std::cerr << "Failed to compile holographic vertex shader" << std::endl;
        return false;
    }

    std::shared_ptr<Eng::FragmentShader> fragmentShader = std::make_shared<Eng::FragmentShader>();
    if (!fragmentShader->load(fragmentShaderCode.c_str())) {
        std::cerr << "Failed to compile holographic fragment shader" << std::endl;
        return false;
    }

    // create and bind program
    holographicShader = std::make_shared<Eng::Program>();
    holographicShader->bindAttribute(ShaderManager::POSITION_LOCATION, "in_Position");
    holographicShader->bindAttribute(ShaderManager::NORMAL_LOCATION, "in_Normal");
    holographicShader->bindAttribute(ShaderManager::TEX_COORD_LOCATION, "in_TexCoord");

    if (!holographicShader->addShader(vertexShader).addShader(fragmentShader).build()) {
        std::cerr << "Failed to link holographic shader program" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Applies the holographic material during rendering.
 *        Binds shader and sets all uniforms for visual effect.
 */
void Eng::HolographicMaterial::render()
{
    auto& sm = ShaderManager::getInstance();

    bool wasBlending = glIsEnabled(GL_BLEND);
    GLint prevSrc = 0, prevDst = 0;

    if (getAlpha() < 1.0f) {
        if (wasBlending) {
            glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrc);
            glGetIntegerv(GL_BLEND_DST_RGB, &prevDst);
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (!holographicShader || !holographicShader->getGlId()) {
        Material::render();
        return;
    }

    //activate program
    if (!sm.loadProgram(holographicShader)) {
        Material::render();
        return;
    }

    // retrieves cached values
    sm.setProjectionMatrix(sm.getCachedProjectionMatrix());
    sm.setModelViewMatrix(sm.getCachedModelViewMatrix());
    sm.setNormalMatrix(sm.getCachedNormalMatrix());
    sm.setLightSpaceMatrix(sm.getCachedLightSpaceMatrix());
    sm.setEyeFront(sm.getCachedEyeFront());
    sm.setGlobalLightColor(sm.getCachedGlobalLightColor());

    GLboolean prevDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    glDepthMask(GL_FALSE);
    
    // no texture
    sm.setUseTexture(false);

    // specific uniforms
    const float t = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()).count() - startTime;

    auto loc = holographicShader->getParamLocation("baseColor");
    if (loc >= 0) holographicShader->setVec3(loc, baseColor);

    loc = holographicShader->getParamLocation("secondaryColor");
    if (loc >= 0) holographicShader->setVec3(loc, secondaryColor);

    loc = holographicShader->getParamLocation("alpha");
    if (loc >= 0) holographicShader->setFloat(loc, getAlpha());

    loc = holographicShader->getParamLocation("bandFrequency");
    if (loc >= 0) holographicShader->setFloat(loc, bandFrequency);

    loc = holographicShader->getParamLocation("bandSpeed");
    if (loc >= 0) holographicShader->setFloat(loc, bandSpeed);

    loc = holographicShader->getParamLocation("time");
    if (loc >= 0) holographicShader->setFloat(loc, t);
    if (getAlpha() < 1.0f) {
        if (wasBlending) {
            glEnable(GL_BLEND);
            glBlendFunc(prevSrc, prevDst);
        }
        else {
            glDisable(GL_BLEND);
        }
    }
    glDepthMask(prevDepthMask); 
}


/**
 * @brief Sets the base color of the holographic material.
 * @param color New base color.
 */
void Eng::HolographicMaterial::setBaseColor(const glm::vec3& color) {
    baseColor = color;
}

/**
 * @brief Retrieves the current base color of the material.
 * @return The current base color.
 */
glm::vec3 Eng::HolographicMaterial::getBaseColor() const {
    return baseColor;
}

/**
 * @brief Sets the frequency of the animated horizontal bands.
 * @param frequency Number of cycles per unit.
 */
void Eng::HolographicMaterial::setBandFrequency(float frequency) {
    bandFrequency = frequency;
}

/**
 * @brief Gets the frequency of the animated horizontal bands.
 * @return The band frequency.
 */
float Eng::HolographicMaterial::getBandFrequency() const {
    return bandFrequency;
}

/**
 * @brief Sets the vertical scrolling speed of the bands.
 * @param speed Scrolling speed multiplier.
 */
void Eng::HolographicMaterial::setBandSpeed(float speed) {
    bandSpeed = speed;
}


/**
 * @brief Gets the scrolling speed of the bands.
 * @return The band speed.
 */
float Eng::HolographicMaterial::getBandSpeed() const {
    return bandSpeed;
}

/**
 * @brief Sets the secondary color used in the band pattern.
 * @param color New secondary color.
 */
void Eng::HolographicMaterial::setSecondaryColor(const glm::vec3& color) {
    secondaryColor = color;
}
/**
 * @brief Gets the current secondary color.
 * @return The secondary color used for band highlights.
 */
glm::vec3 Eng::HolographicMaterial::getSecondaryColor() const {
    return secondaryColor;
}