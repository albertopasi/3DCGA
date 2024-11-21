#include "terrain.h"

Terrain::~Terrain()
{
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);
}

Terrain::Terrain() : Terrain(terrainWidth, terrainResolution) {}

Terrain::Terrain(unsigned int tWidth, unsigned int tRes)
        :heightMap(), snow(RESOURCE_ROOT "resources/terrain1/textureSnow.png"), grass(RESOURCE_ROOT "resources/terrain1/textureGrass.png"), 
        ground(RESOURCE_ROOT "resources/terrain1/textureGround.jpg"), rock(RESOURCE_ROOT "resources/terrain2/textureRock.jpg") {
    terrainWidth = tWidth;
    terrainResolution = tRes;

    glm::mat4 id;
    glm::mat4 scaleMatrix = glm::scale(id, glm::vec3(1.0, 0.0, 1.0));
    glm::mat4 positionMatrix = glm::translate(id, glm::vec3(0., 0.0, 0.));
    modelMatrix = positionMatrix;

    initBuffers();

    //add shaders
    ShaderBuilder terrainBuilder;
    terrainBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/terrain_vert.glsl");
    terrainBuilder.addStage(GL_TESS_CONTROL_SHADER, RESOURCE_ROOT "shaders/terrain_tcs.glsl");
    terrainBuilder.addStage(GL_TESS_EVALUATION_SHADER, RESOURCE_ROOT "shaders/terrain_tes.glsl");
    terrainBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/terrain_frag.glsl");
    terrainShader = terrainBuilder.build();
}

// Generate terrain
std::vector<Vertex> Terrain::generateTerrain(float width, int resolution) {
    std::vector<Vertex> vertices;
    float halfWidth = width / 2.0f;
    float step = width / static_cast<float>(resolution);  // Step between vertices

    // For to create vertices
    for (int z = 0; z <= resolution; ++z) {
        for (int x = 0; x <= resolution; ++x) {
            Vertex vertex;
            vertex.position = glm::vec3(-halfWidth + x * step, 0.0f, -halfWidth + z * step);  // Position on plane XZ
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Normal
            vertex.texCoord = glm::vec2(static_cast<float>(x) / resolution, static_cast<float>(z) / resolution);  // texture coordinate

            vertices.push_back(vertex);
        }
    }

    return vertices;
}

std::vector<float> Terrain::generateTerrainVertices(float width, float height, unsigned int resolution) {
    std::vector<float> vertices;

    for (unsigned int i = 0; i < resolution; ++i) {
        for (unsigned int j = 0; j < resolution; ++j) {
            // Vertice 1 of quad
            vertices.push_back(-width / 2.0f + width * i / (float)resolution);  // X
            vertices.push_back(0.0f);                                           // Y (height to be determined in the TES)
            vertices.push_back(-height / 2.0f + height * j / (float)resolution); // Z
            vertices.push_back(i / (float)resolution);                          // Texture Coord U
            vertices.push_back(j / (float)resolution);                          // Texture Coord V

            // Vertice 2 of quad
            vertices.push_back(-width / 2.0f + width * (i + 1) / (float)resolution);  // X
            vertices.push_back(0.0f);                                                // Y
            vertices.push_back(-height / 2.0f + height * j / (float)resolution);      // Z
            vertices.push_back((i + 1) / (float)resolution);                         // U
            vertices.push_back(j / (float)resolution);                               // V

            // Vertice 3 of quad
            vertices.push_back(-width / 2.0f + width * i / (float)resolution);  // X
            vertices.push_back(0.0f);                                           // Y
            vertices.push_back(-height / 2.0f + height * (j + 1) / (float)resolution); // Z
            vertices.push_back(i / (float)resolution);                          // U
            vertices.push_back((j + 1) / (float)resolution);                    // V

            // Vertice 4 of quad
            vertices.push_back(-width / 2.0f + width * (i + 1) / (float)resolution);  // X
            vertices.push_back(0.0f);                                                // Y
            vertices.push_back(-height / 2.0f + height * (j + 1) / (float)resolution); // Z
            vertices.push_back((i + 1) / (float)resolution);                         // U
            vertices.push_back((j + 1) / (float)resolution);                         // V
        }
    }

    return vertices;
}

void Terrain::initBuffers() {
    // Generate vertices
    std::vector<float> vertices = generateTerrainVertices(terrainWidth, terrainWidth, terrainResolution);

    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);

    glBindVertexArray(terrainVAO);

    // Loads vertex data in the VBO
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    // Position attribute (X, Y, Z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinates attribute (U, V)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Terrain::draw(glm::mat4 mvpMatrix, glm::mat4 viewMatrix, glm::vec3 cameraPos, glm::vec3 lightPos) {

    terrainShader.bind();
    glUniformMatrix4fv(terrainShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, value_ptr(mvpMatrix));

    // Sets the uniform for lighting and camera's position
    glUniform3fv(terrainShader.getUniformLocation("camPos"), 1, glm::value_ptr(cameraPos));

    heightMap.bind(GL_TEXTURE8);
    glUniform1i(terrainShader.getUniformLocation("heightMap"), 8);

    snow.bind(GL_TEXTURE11);
    glUniform1i(terrainShader.getUniformLocation("textureSnow"), 11);
    grass.bind(GL_TEXTURE12);
    glUniform1i(terrainShader.getUniformLocation("textureGrass"), 12);
    ground.bind(GL_TEXTURE13);
    glUniform1i(terrainShader.getUniformLocation("textureGround"), 13);
    rock.bind(GL_TEXTURE14);
    glUniform1i(terrainShader.getUniformLocation("textureRock"), 14);

    // Render the terrain using glDrawElements
    glBindVertexArray(terrainVAO);
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, terrainResolution * terrainResolution * 4);
    glBindVertexArray(0);

}

void Terrain::setGui()
{
    ImGui::Begin("Terrain controls: ");
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Terrain Controls");
    ImGui::SliderInt("Terrain Width", &terrainWidth, 10, 50);
    ImGui::SliderInt("Terrain Resolution", &terrainResolution, 100, 3000);

    ImGui::End();
}

// Function to generate terrain indexes
std::vector<unsigned int> Terrain::generateTerrainIndices(int resolution) {
    std::vector<unsigned int> indices;

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            // quad's vertices (4 vertices)
            unsigned int topLeft = z * (resolution + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (resolution + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return indices;
}

float Terrain::heightAtPos(float x, float y){
    int i = glm::floor((x + terrainWidth / 2.0) * terrainResolution / terrainWidth);
    int j = glm::floor((y + terrainWidth / 2.0) * terrainResolution / terrainWidth);
    float u = i / (float)terrainResolution;
    float v = j / (float)terrainResolution;
    float height = getBilinearNoiseValueAtUV(u, v, heightMap.getPerlinNoise(), heightMap.getTexSize(), heightMap.getTexSize());
    return height*8;
}

float Terrain::getBilinearNoiseValueAtUV(float u, float v, const std::vector<float>& noiseData, int width, int height) {
    u = fmod(u, 1.0f);
    if (u < 0) u += 1.0f;
    v = fmod(v, 1.0f);
    if (v < 0) v += 1.0f;

    float x = u * width;
    float y = v * height;
    
    int x0 = static_cast<int>(x) % width;
    int x1 = (x0 + 1) % width;
    int y0 = static_cast<int>(y) % height;
    int y1 = (y0 + 1) % height;

    float dx = x - x0;
    float dy = y - y0;

    // Get the values at the four corners
    float v00 = noiseData[y0 * width + x0];
    float v10 = noiseData[y0 * width + x1];
    float v01 = noiseData[y1 * width + x0];
    float v11 = noiseData[y1 * width + x1];

    // Bilinear interpolation
    float v0 = v00 * (1 - dx) + v10 * dx;
    float v1 = v01 * (1 - dx) + v11 * dx;
    return v0 * (1 - dy) + v1 * dy;
}