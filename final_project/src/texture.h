#pragma once
#include <vector>
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>
#include <unordered_map>

//Exception class for handling image loading errors
struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Texture {
public:
    // Constructor that loads the texture from a file
    Texture(std::filesystem::path filePath);
    Texture(const Texture&) = delete;
    Texture(Texture&&);
    ~Texture();

    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&&) = default;

    //Binds the texture to a specific texture slot
    void bind(GLint textureSlot);

private:

    // Invalid texture identifier
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture { INVALID };
};

//Class to handle PBR Texture
class PBRTextures {

public:
    //Constructor to initialize PBR textures from a directory
    PBRTextures(const std::filesystem::path &directoryPath);
    // Loads textures from the specified directory
    void load(const std::filesystem::path& directoryPath);

public:
    // Unordered map to store textures by their type
    std::unordered_map<std::string, Texture> textures;
    std::filesystem::path textureDirectory;
    bool isMetallic = false;
    // Retrieves a specific texture based on its type
    Texture& getTexture(const std::string& textureType);


private:
    // List of texture types in PBR materials
    std::vector<std::string> texturePrefixes = { "albedo","roughness", "normal", "ao", "metallic"};
};

//Class to handle CubeMap textures for environmental mapping
class CubeMapTexture {
public:
    // Constructor that loads a cube map from a directory
    CubeMapTexture(const std::filesystem::path& directoryPath);
    CubeMapTexture(const CubeMapTexture&) = delete;
    CubeMapTexture(CubeMapTexture&&);
    ~CubeMapTexture();

    CubeMapTexture& operator=(const CubeMapTexture&) = delete;
    CubeMapTexture& operator=(CubeMapTexture&&) = default;

    // Binds the cube map texture to a specific texture slot
    void bind(GLint textureSlot);

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture { INVALID };

    // Names of the faces of the cube map
    std::vector<std::string> faces = {"right", "left", "top", "bottom", "front", "back"};
};

// Class to handle a random texture
class RandomTexture {
public:
    // Default constructor for generating a random texture
    RandomTexture();
    RandomTexture(const RandomTexture&) = delete;
    RandomTexture(RandomTexture&&);
    ~RandomTexture();

    RandomTexture& operator=(const RandomTexture&) = delete;
    RandomTexture& operator=(RandomTexture&&) = default;
    
    void bind(GLint textureSlot);

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture { INVALID };
};

class HeightMapTexture {
public:
    HeightMapTexture();
    HeightMapTexture(const HeightMapTexture&) = delete;
    HeightMapTexture(HeightMapTexture&&);
    ~HeightMapTexture();

    HeightMapTexture& operator=(const HeightMapTexture&) = delete;
    HeightMapTexture& operator=(HeightMapTexture&&) = default;
    
    void bind(GLint textureSlot);
    std::vector<float> getPerlinNoise() { return perlinNoise; }
    int getTexSize(){ return texSize; }

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture { INVALID };
    std::vector<float> perlinNoise;
    int texSize;

    float fractalBrownianMotion(int x, int y, int numOctaves, float scale, float persistance, float lacunarity, glm::vec2* octaveOffset);
    float randomNumber();
    float interpolate(float a0, float a1, float w);
    glm::vec2 randomGradient(int ix, int iy);
    float dotGridGradient(int ix, int iy, float x, float y);
    float perlin(float x, float y);
    float inverseLerp(float a, float b, float value);
};

class ShadowMap {

public:
    ShadowMap();
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap(ShadowMap&&);
    ~ShadowMap();

    ShadowMap& operator=(const ShadowMap&) = delete;
    ShadowMap& operator=(ShadowMap&&) = default;

    void init();
    void bindTexture(GLint textureSlot);
    void bindFramebuffer();

    GLuint getTexture(){ return m_texture; };

private:
    static constexpr int SHADOWTEX_WIDTH = 4096;
    static constexpr int SHADOWTEX_HEIGHT = 4096;
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture { INVALID };
    GLuint m_frameBuffer { INVALID };
};