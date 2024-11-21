#include "minimap.h"

// Constructor: allows custom width and height, defaults to 200x200
Minimap::Minimap(int width, int height) : width(width), height(height) {
    // Initialize framebuffer and shader
    init();
}

// Destructor: Clean up resources
Minimap::~Minimap() {
    glDeleteVertexArrays(1, &minimapVAO);
    glDeleteBuffers(1, &minimapVBO);
}

// Initialize VAO and VBO, pass shader
void Minimap::init() {

    ShaderBuilder minimapBuilder;
    minimapBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/minimap_vert.glsl");
    minimapBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/minimap_frag.glsl");
    minimapShader = minimapBuilder.build();
    
    // Create and initialize the VAO and VBO for the minimap quad
    glGenVertexArrays(1, &minimapVAO);
    glGenBuffers(1, &minimapVBO);
    glBindVertexArray(minimapVAO);

    float minimapVertices[] = {
        // Positions        // Texture Coordinates
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        -0.5f, -1.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  1.0f, 1.0f,

        -0.5f, -0.5f, 0.0f,  1.0f, 1.0f,
        -1.0f,  -0.5f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, minimapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(minimapVertices), minimapVertices, GL_STATIC_DRAW);

    // Set up vertex attributes (position and texture coordinates)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO
}

// Draw the minimap on the screen as a quad with the texture applied
void Minimap::drawMinimap(GLuint texture) {
    minimapShader.bind();  // Use the minimap shader
    glBindVertexArray(minimapVAO);
    glDisable(GL_DEPTH_TEST);  // Disable depth testing for the minimap

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUniform1i(minimapShader.getUniformLocation("minimapTexture"), 10);

    // Draw the quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);  // Re-enable depth testing for normal rendering
}
