#pragma once

/**
 * @class Skybox
 * @brief Renders a cubemap background and provides ambient scene color.
 *
 * The Skybox class loads six textures into an OpenGL cubemap, sets up a cube
 * mesh (VAO/VBO), and uses a specialized shader program to draw an infinitely
 * distant environment. It also calculates a weighted average color from the
 * cubemap faces to supply a global ambient color for scene lighting.
 */
class ENG_API Skybox : public Eng::Node {
public:
    // faces: list of six image file paths for posx, negx, posy, negy, posz, negz.
    Skybox(const std::vector<std::string>& faces);
    virtual ~Skybox();

    // Initializes the cubemap, VAO/VBO, and shader.
    bool init();

    // Render the skybox.
    void render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    // Get global color
    glm::vec3 getGlobalColor();

private:
    // File names for each cube face.
    std::vector<std::string> faces;

    // OpenGL handles.
    unsigned int cubemapTexture = 0;
    unsigned int vao = 0;
    unsigned int vbo = 0;

    std::shared_ptr<Eng::Program> skyboxProgram;

    // Helper to load the cubemap texture.
    bool loadCubemap();

	glm::vec3 globalColor = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 calculateWeightedAverageColor(unsigned char* bits, int width, int height, int channels = 3);
    glm::vec3 calculateWeightedAverageColorHDR(float* floatBits, int width, int height, int channels);
};