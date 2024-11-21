#include "texture.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cmath>
#include <random>

Texture::Texture(std::filesystem::path filePath)
{
    // Load image from disk to CPU memory.
    // Image class is defined in <framework/image.h>
    Image cpuTexture { filePath };

    // Create a texture on the GPU and bind it for parameter setting
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set behavior for when texture coordinates are outside the [0, 1] range (wrap around).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set interpolation for texture sampling (bilinear interpolation across mip-maps).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Define GPU texture parameters and upload corresponding data based on number of image channels
    switch (cpuTexture.channels) {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, cpuTexture.width, cpuTexture.height, 0, GL_RED, GL_UNSIGNED_BYTE, cpuTexture.get_data());
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cpuTexture.width, cpuTexture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, cpuTexture.get_data());
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cpuTexture.width, cpuTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cpuTexture.get_data());
            break;
        default:
            std::cerr << "Number of channels read for texture is not supported" << std::endl;
            throw std::exception();
    }

    // Generate mip-maps
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(Texture&& other)
    : m_texture(other.m_texture)
{
    other.m_texture = INVALID;
}

Texture::~Texture()
{
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}

void Texture::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

//PBR Texture class constructor
PBRTextures::PBRTextures(const std::filesystem::path &directoryPath) {
    load(directoryPath);
}

//Load textures from the directory path
void PBRTextures::load(const std::filesystem::path& directoryPath) {

    if(textureDirectory.compare(directoryPath) == 0)     // same path. Already loaded
        return;

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(fmt::format("{} is not a directory", directoryPath.string()));
    }

    textures.clear(); // Clear previously loaded textures
    isMetallic = false; // Reset metallic flag

    // Check if the filename starts with any of the predefined texture prefixes
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();

            for (const auto& prefix : texturePrefixes) {
                if (filename.rfind(prefix, std::string::npos) != std::string::npos) {
                    try {
                        // Add texture to the map
                        textures.insert_or_assign(prefix, Texture(entry.path()));
                        std::cout << "Loaded texture: " << filename << " as " << prefix << std::endl;
                        if(prefix == "metallic")
                            isMetallic = true; // Mark as metallic if the texture is metallic
                    } catch (const std::exception& e) {
                        std::cerr << "Error loading texture " << filename << ": " << e.what() << std::endl;
                    }
                    break;
                }
            }
        }
    }

    if (textures.empty()) {
        throw std::runtime_error("No valid textures found in the directory");
    }
    textureDirectory = directoryPath;
}

// Retrieves a specific texture based on its type
Texture& PBRTextures::getTexture(const std::string& textureType) {
    auto it = textures.find(textureType);
    if (it != textures.end()) {
        return it->second;
    } else {
        throw std::runtime_error(fmt::format("Texture of type {} not found", textureType));
    }
}

// Constructor that loads a cube map from a directory
CubeMapTexture::CubeMapTexture(const std::filesystem::path& directoryPath) {

    if (!std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error(fmt::format("{} is not a directory", directoryPath.string()));
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);

    // Set texture parameters for the cube map
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Iterate through the directory and load the faces of the cube map
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            int i = 0;
            for (const auto& prefix : faces) {
                if (filename.rfind(prefix, 0) == 0) {  // If the filename starts with the prefix
                    try {                            
                        std::cout << "Loaded texture: " << filename << " as " << prefix << std::endl;
                        Image cpuTexture { entry.path() };

                        // // Upload the texture data based on the number of channels
                        switch (cpuTexture.channels) {
                            case 1:
                                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RED, cpuTexture.width, cpuTexture.height, 0, GL_RED, GL_UNSIGNED_BYTE, cpuTexture.get_data());
                                break;
                            case 3:
                                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, cpuTexture.width, cpuTexture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, cpuTexture.get_data());
                                break;
                            case 4:
                                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, cpuTexture.width, cpuTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cpuTexture.get_data());
                                break;
                            default:
                                std::cerr << "Number of channels read for texture is not supported" << std::endl;
                                throw std::exception();
                        }

                    } catch (const std::exception& e) {
                        std::cerr << "Error loading texture " << filename << ": " << e.what() << std::endl;
                    }
                    break;
                }
                i++;
            }
        }
    }
}


CubeMapTexture::CubeMapTexture(CubeMapTexture&& other) 
    : m_texture(other.m_texture) {
    other.m_texture = INVALID;
}

//Destructor for CubeMapTexture
CubeMapTexture::~CubeMapTexture()
{
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}

void CubeMapTexture::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
}

//RandomTexture constructor: Generates random texture data and uploads it
RandomTexture::RandomTexture()
{
    std::srand(std::time(0));

    glm::vec3* pRandomData = new glm::vec3[1000]; // Seed the random number generator

    for (unsigned int i = 0 ; i < 1000 ; i++) { // Allocate memory for random texture data
        pRandomData[i].x = ((float)std::rand() / RAND_MAX);
        pRandomData[i].y = ((float)std::rand() / RAND_MAX);
        pRandomData[i].z = ((float)std::rand() / RAND_MAX);
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_1D, m_texture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 1000, 0, GL_RGB, GL_FLOAT, pRandomData);
    glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // Wrap the texture around
    
    delete [] pRandomData; // Clean up dynamically allocated memory
}  
  
RandomTexture::RandomTexture(RandomTexture&& other)
    : m_texture(other.m_texture)
{
    other.m_texture = INVALID;
}  

// Destructor for RandomTexture
RandomTexture::~RandomTexture()
{
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}
  
void RandomTexture::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_1D, m_texture);
}

HeightMapTexture::HeightMapTexture()
{
    int numOctaves = 4;

    int seed = 1;
    texSize = 512;
    float scale = 70.0;
    float persistance = 0.5;
    float lacunarity = 2;

    float minHeight = FLT_MAX;
    float maxHeight = FLT_MIN;

    perlinNoise = std::vector<float>(texSize * texSize);

    glm::vec2 octaveOffset[numOctaves];
    for(int i = 0; i < numOctaves; i++){
        std::mt19937 gen(seed); 
        std::uniform_real_distribution<float> dist(-100000.0f, 100000.0f);
        float x = dist(gen);
        float y = dist(gen);
        octaveOffset[i] = glm::vec2(x, y);
    }
    
    for(int i = 0; i < texSize; i++){
        for(int j = 0; j < texSize; j++){
            float height = fractalBrownianMotion(i, j, numOctaves, scale, persistance, lacunarity, octaveOffset);
            perlinNoise[i * texSize + j] = height;
            if (height > maxHeight) maxHeight = height;
            else if (height < minHeight) minHeight = height;
        }
    }

    for(int i = 0; i < texSize; i++){
        for(int j = 0; j < texSize; j++){
            perlinNoise[i * texSize + j] = inverseLerp(minHeight, maxHeight, perlinNoise[i * texSize + j]);
        }
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texSize, texSize, 0, GL_RED, GL_FLOAT, perlinNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set interpolation for texture sampling (bilinear interpolation across mip-maps).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
}  
  
HeightMapTexture::HeightMapTexture(HeightMapTexture&& other)
    : m_texture(other.m_texture)
{
    other.m_texture = INVALID;
}  
  
HeightMapTexture::~HeightMapTexture()
{
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}
  
void HeightMapTexture::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

/* Function to linearly interpolate between a0 and a1
 * Weight w should be in the range [0.0, 1.0]
 */
float HeightMapTexture::interpolate(float a0, float a1, float w) {
    /* // You may want clamping by inserting:
     * if (0.0 > w) return a0;
     * if (1.0 < w) return a1;
     */
    return (a1 - a0) * w + a0;
    /* // Use this cubic interpolation [[Smoothstep]] instead, for a smooth appearance:
     * return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
     *
     * // Use [[Smootherstep]] for an even smoother result with a second derivative equal to zero on boundaries:
     * return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
     */
}

/* Create pseudorandom direction vector
 */
glm::vec2 HeightMapTexture::randomGradient(int ix, int iy) {
    // No precomputed gradients mean this works for any number of grid coordinates
    const unsigned w = 8 * sizeof(unsigned);
    const unsigned s = w / 2; // rotation width
    unsigned a = ix, b = iy;
    a *= 3284157443; b ^= a << s | a >> w-s;
    b *= 1911520717; a ^= b << s | b >> w-s;
    a *= 2048419325;
    float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
    glm::vec2 v = glm::vec2(cos(random), sin(random));
    //v.x = cos(random); v.y = sin(random);
    return v;
}

// Computes the dot product of the distance and gradient vectors.
float HeightMapTexture::dotGridGradient(int ix, int iy, float x, float y) {
    // Get gradient from integer coordinates
    glm::vec2 gradient = randomGradient(ix, iy);

    // Compute the distance vector
    float dx = x - (float)ix;
    float dy = y - (float)iy;

    // Compute the dot-product
    return (dx*gradient.x + dy*gradient.y);
}

// Compute Perlin noise at coordinates x, y
float HeightMapTexture::perlin(float x, float y) {
    // Determine grid cell coordinates
    int x0 = (int)floor(x);
    int x1 = x0 + 1;
    int y0 = (int)floor(y);
    int y1 = y0 + 1;

    // Determine interpolation weights
    // Could also use higher order polynomial/s-curve here
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = interpolate(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return value; // Will return in range -1 to 1. To make it in range 0 to 1, multiply by 0.5 and add 0.5
}

float HeightMapTexture::fractalBrownianMotion(int i, int j, int numOctaves, float scale, float persistance, float lacunarity, glm::vec2* octaveOffset){
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float noiseHeight = 0.0f;

    if(scale <= 0.0) scale = 0.00001f;

    for(int k = 0; k < numOctaves; k++){
        float sampleX = i / scale * frequency + octaveOffset[k][0];
        float sampleY = j / scale * frequency + octaveOffset[k][1];
        float perlinValue = perlin(sampleX, sampleY) * 2 - 1;
        noiseHeight += perlinValue * amplitude;
        amplitude *= persistance;
        frequency *= lacunarity; 
    }

    return noiseHeight;
}

float HeightMapTexture::inverseLerp(float a, float b, float value) {
    if (a != b) { // Evita la divisione per zero
        return (value - a) / (b - a);
    } else {
        return 0.0f; // Se a e b sono uguali, restituisce 0
    }
}

ShadowMap::ShadowMap(){
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Set behaviour for when texture coordinates are outside the [0, 1] range.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glGenFramebuffers(1, &m_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
}

ShadowMap::ShadowMap(ShadowMap&& other)
    : m_texture(other.m_texture)
{
    other.m_texture = INVALID;
}

ShadowMap::~ShadowMap(){
    if (m_texture != INVALID){
        glDeleteTextures(1, &m_texture);
        glDeleteBuffers(1, &m_frameBuffer);
    }
}

void ShadowMap::bindTexture(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void ShadowMap::bindFramebuffer(){
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
}

void ShadowMap::init(){}