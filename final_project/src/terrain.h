#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <framework/shader.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "framework/mesh.h"
#include "Camera.h"
#include "texture.h"
#include "utils.h"

const int TERRAIN_WIDTH = 10;  // Can be changed based on needs
const int TERRAIN_RESOLUTION = 100; // Number of divisions in the grid ( higher = higher resolution)

class Terrain {

public:
    Terrain();
    Terrain(unsigned int tWidth, unsigned int tHeight);

    ~Terrain();


    void initBuffers();
    void draw(glm::mat4 mvpMatrix, glm::mat4 normalModelMatrix, glm::vec3 cameraPos, glm::vec3 lightPos);
    std::vector<float> generateTerrainVertices(float width, float height, unsigned int resolution);
    std::vector<Vertex> generateTerrain(float width, int resolution);
    std::vector<unsigned int> generateTerrainIndices(int resolution);
    float heightAtPos(float x, float y);
    void setGui();


    GLuint terrainVAO, terrainVBO, terrainEBO;

    Shader terrainShader;

    HeightMapTexture heightMap;
    Texture snow;
    Texture grass;
    Texture ground;
    Texture rock;

    int terrainWidth = TERRAIN_WIDTH;
    int terrainResolution = TERRAIN_RESOLUTION;

    glm::mat4 modelMatrix;
    std::vector<Vertex> terrainVertices;
    std::vector<unsigned int> terrainIndices;
private:
    float getBilinearNoiseValueAtUV(float u, float v, const std::vector<float>& noiseData, int width, int height);
};

#endif //TERRAIN_H
