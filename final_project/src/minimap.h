#ifndef MINIMAP_H
#define MINIMAP_H

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

const int MINIMAP_WIDTH = 200;  // Default minimap width in pixels
const int MINIMAP_HEIGHT = 200; // Default minimap height in pixels

class Minimap {
public:
    // Constructor with default size 200
    Minimap(int width = MINIMAP_WIDTH, int height = MINIMAP_HEIGHT);  
    ~Minimap();  // Destructor

    void init();  // Initialize VAO, VBO and pass shader
    void drawMinimap(GLuint texture);  // Draw the minimap on the screen
   
private:
    GLuint minimapVAO, minimapVBO;
    Shader minimapShader;
    int width, height;  // Minimap width and height
};

#endif
