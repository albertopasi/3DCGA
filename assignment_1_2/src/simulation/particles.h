#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <framework/opengl_includes.h>
#include <glm/mat4x4.hpp>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>

#include <render/mesh.h>
#include <utils/config.h>

#include <stdint.h>


class ParticlesSimulator {
public:
    ParticlesSimulator(Config& config);
    ~ParticlesSimulator();

    void render(const glm::mat4& viewProjection);
    void resetSimulation();

private:
    // Shared state
    Config& config;

    // Internal variables
    bool renderToPing = true;                                                   // Indicates which framebuffer the simulation step will render to
    GLuint simulationFramebufferPing, simulationFramebufferPong;                // Framebuffers rendered to in our mock compute shader
    GLuint positionTexPing, velocityTexPing, positionTexPong, velocityTexPong;  // Textures storing per-particle position and velocity data
    GLuint bouncesTexPing, bouncesTexPong;                                      // Textures storing per-particle collision counting data (R channel is number of bounces, G channel is number of frames left for the bounce color to be active)
    Shader initialDataPass, drawPass, simulationPass;
    GPUMesh particleModel;

    // Framebuffer and texture management
    void initFramebuffersAndTextures();
    void setInitialData();
    void deleteFramebuffersAndTextures();

    // Misc setup
    void initShaders();

    // Main loop
    void draw(const glm::mat4& viewProjection);
    void simulate();
};
