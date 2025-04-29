#include "engine.h"

#include "FreeImage.h"
#include <GL/glew.h>

// Cube vertices for the skybox (36 vertices for a cube)
static const float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

/// Skybox vertex shader source.
static const char* skyboxVertShaderSrc = R"(
    #version 440 core
    layout(location = 0) in vec3 in_Position;
    
    out vec3 TexCoords;
    
    uniform mat4 projection;
    uniform mat4 view;
    
    void main()
    {
        TexCoords = in_Position;
        gl_Position = projection * view * vec4(in_Position, 1.0);
    }
)";

/// Skybox fragment shader source.
static const char* skyboxFragShaderSrc = R"(
    #version 440 core
    in vec3 TexCoords;
    out vec4 fragColor;
    
    uniform samplerCube skybox;
    
void main() {    
    vec3 color = texture(skybox, TexCoords).rgb;
    
    //amplify colors (il risultato viene piu carino cosi)
    color *= 3.0; 
    
    // Amplify luminance
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (luminance > 0.6) {
        color *= 1.0 + (luminance - 0.6) * 2.0; // Amplifica ancora di più aree luminose
    }
    
    fragColor = vec4(color, 1.0);
}
)";

Eng::Skybox::Skybox(const std::vector<std::string>& faces)
    : faces(faces)
{
}

Eng::Skybox::~Skybox()
{
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }
    if (cubemapTexture) {
        glDeleteTextures(1, &cubemapTexture);
    }
}

bool Eng::Skybox::init()
{
    // Load the cubemap texture.
    if (!loadCubemap()) {
        std::cerr << "[Skybox] Failed to load cubemap texture." << std::endl;
        return false;
    }

    // Generate VAO and VBO for the cube.
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Build the skybox shader program.
    std::shared_ptr<VertexShader> vs = std::make_shared<VertexShader>();
    if (!vs->load(skyboxVertShaderSrc)) {
        std::cerr << "[Skybox] Failed to load vertex shader source." << std::endl;
        return false;
    }

    std::shared_ptr<FragmentShader> fs = std::make_shared<FragmentShader>();
    if (!fs->load(skyboxFragShaderSrc)) {
        std::cerr << "[Skybox] Failed to load fragment shader source." << std::endl;
        return false;
    }

    std::shared_ptr<Program> program = std::make_shared<Program>();
    program->bindAttribute(0, "in_Position");
    program->bindSampler(0, "skybox");

    program->addShader(vs);
    program->addShader(fs);

    if (!program->build()) {
        std::cerr << "[Skybox] Failed to build the shader program." << std::endl;
        return false;
    }
    // Directly store the program as our skyboxProgram.
    skyboxProgram = program;

    return true;
}

bool Eng::Skybox::loadCubemap()
{
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // Set wrapping and filtering parameters.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::vec3 faceAverageColorSum(0.0f);
    int faceCount = faces.size();

    // Load each of the six faces.
    for (unsigned int i = 0; i < faces.size(); i++) {
        const char* file = faces[i].c_str();
        FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(file, 0);
        if (fif == FIF_UNKNOWN) {
            fif = FreeImage_GetFIFFromFilename(file);
        }
        if (fif == FIF_UNKNOWN) {
            std::cerr << "[Skybox] Unable to determine format for " << file << std::endl;
            return false;
        }
        FIBITMAP* dib = FreeImage_Load(fif, file);
        if (!dib) {
            std::cerr << "[Skybox] Failed to load image " << file << std::endl;
            return false;
        }
        BYTE* bits = FreeImage_GetBits(dib);
        int width = FreeImage_GetWidth(dib);
        int height = FreeImage_GetHeight(dib);

        // Determine internal format and pixel data format.
        // FreeImage stores pixel data in BGR(A) format, but we want OpenGL to store it internally as RGB(A).
        GLenum internalFormat;
        GLenum format;
        int channels;
        GLenum type = GL_UNSIGNED_BYTE;
        if (FreeImage_GetBPP(dib) == 32) {
            internalFormat = GL_RGBA;
            format = GL_BGRA;
			channels = 4;
        }
        else if (FreeImage_GetBPP(dib) == 128 || FreeImage_GetBPP(dib) == 96) { // 32 bit per canale (floating point HDR)
            internalFormat = GL_RGB16F;
            format = GL_RGB;
            type = GL_FLOAT;
            channels = 3;
        }
        else {
            internalFormat = GL_RGB;
            format = GL_BGR;
			channels = 3;
        }

        FreeImage_FlipVertical(dib);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height,
            0, format, type, bits);

        glm::vec3 faceAverageColor;
        if (type == GL_FLOAT) { //HDR
            float* floatBits = (float*)bits;
            faceAverageColor = calculateWeightedAverageColorHDR(floatBits, width, height, channels);
        }
        else { //LDR
            faceAverageColor = calculateWeightedAverageColor(bits, width, height, channels);
        }
        faceAverageColorSum += faceAverageColor;

        FreeImage_Unload(dib);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// Compute average color of all faces
	glm::vec3 globalAverageColor = faceAverageColorSum / static_cast<float>(faceCount);

	// Set the global color based on the average color of the skybox
    globalColor = glm::vec3{ globalAverageColor.r * 0.2f, globalAverageColor.g * 0.2f, globalAverageColor.b * 0.2f };
    return true;
}


void Eng::Skybox::render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    GLint prevProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLboolean prevDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);

    // Set depth function so that skybox fragments with equal depth pass.
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    // Activate the skybox shader program.
    skyboxProgram->render();

    // Remove the translation from the view matrix so that the skybox appears fixed.
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));

    // Retrieve uniform locations and set the matrices.
    int viewLoc = skyboxProgram->getParamLocation("view");
    int projLoc = skyboxProgram->getParamLocation("projection");
    skyboxProgram->setMatrix(viewLoc, viewNoTrans);
    skyboxProgram->setMatrix(projLoc, projectionMatrix);

    // Bind the VAO and cubemap texture.
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    // Draw the cube (36 vertices).
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Restore default depth function.
    glUseProgram(prevProgram);        // programma precedente
    glDepthFunc(GL_LESS);
    glDepthMask(prevDepthMask);
}

/// Calculate the average color of the skybox texture using luminance weighting.
/// bits: pixel data
/// width: image width
/// height: image height
/// channels: number of color channels (3 for BGR, 4 for BGRA, default 3)
glm::vec3 Eng::Skybox::calculateWeightedAverageColor(unsigned char* bits, int width, int height, int channels)
{
    float totalLuminance = 0.0f;
    glm::vec3 weightedColor(0.0f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixelIndex = (y * width + x) * channels;

            // Read colors (ignoring alpha if present)
            float r = bits[pixelIndex + 2] / 255.0f;  // Red (index +2 in BGR(A))
            float g = bits[pixelIndex + 1] / 255.0f;  // Green
            float b = bits[pixelIndex + 0] / 255.0f;  // Blue

            float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;

            weightedColor += glm::vec3(r, g, b) * luminance;
            totalLuminance += luminance;
        }
    }

    if (totalLuminance > 0.0f) {
        weightedColor /= totalLuminance;
    }

    return weightedColor;
}
glm::vec3 Eng::Skybox::calculateWeightedAverageColorHDR(float* floatBits, int width, int height, int channels) {
    float totalLuminance = 0.0f;
    glm::vec3 weightedColor(0.0f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixelIndex = (y * width + x) * channels;

            // Read colors (ignoring alpha if present)
            float r = floatBits[pixelIndex + 0];  // Red (index +0 in RGB(A) format for EXR)
            float g = floatBits[pixelIndex + 1];  // Green
            float b = floatBits[pixelIndex + 2];  // Blue

            //Uses tone mapping to map color to 0 -1 range
            if (r > 1.0f || g > 1.0f || b > 1.0f) {
                // log compression
                r = 1.0f + log2(r) * 0.5f;
                g = 1.0f + log2(g) * 0.5f;
                b = 1.0f + log2(b) * 0.5f;
            }

            float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;

            weightedColor += glm::vec3(r, g, b) * luminance;
            totalLuminance += luminance;
        }
    }

    if (totalLuminance > 0.0f) {
        weightedColor /= totalLuminance;
    }

    return weightedColor;
}

glm::vec3 Eng::Skybox::getGlobalColor()
{
	return globalColor;
}
