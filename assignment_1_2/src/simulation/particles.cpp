#include "particles.h"

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()

#include <render/mesh.h>
#include <utils/constants.h>
#include <utils/render_utils.hpp>

#include <array>
#include <iostream>


ParticlesSimulator::ParticlesSimulator(Config& config)
    : config(config)
    , particleModel(utils::RESOURCES_DIR_PATH / "sphere.obj", true) {
    initShaders();
    initFramebuffersAndTextures();
    setInitialData();
}

ParticlesSimulator::~ParticlesSimulator() {
    deleteFramebuffersAndTextures();
}

void ParticlesSimulator::render(const glm::mat4& viewProjection) {
    // Simulation step needed
    if (config.doContinuousSimulation || config.doSingleStep) {
        simulate();             
        renderToPing          = !renderToPing;  // Swap ping-pong buffers so drawing can happen from correct buffer
        config.doSingleStep = false;            // Reset single step flag
    }

    // Draw particles based on current data
    draw(viewProjection);
}

void ParticlesSimulator::resetSimulation() {
    deleteFramebuffersAndTextures();
    initFramebuffersAndTextures();
    setInitialData();
}

void ParticlesSimulator::initFramebuffersAndTextures() {
    // Generate ping and pong framebuffers
    glGenFramebuffers(1, &simulationFramebufferPing);
    glGenFramebuffers(1, &simulationFramebufferPong);

    // Create all textures. Textures have dimensions (numParticles, 1)
    std::array<GLuint*, 6UL> allTexPtrs = { &positionTexPing, &positionTexPong, &velocityTexPing, &velocityTexPong, &bouncesTexPing, &bouncesTexPong};
    for (GLuint* texPtr : allTexPtrs) {
        glGenTextures(1, texPtr);
        glBindTexture(GL_TEXTURE_2D, *texPtr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, config.numParticles, 1, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    // Attach textures to pingpong framebuffers and enable drawing to them
    std::array<GLuint, 3UL> attachments { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glBindFramebuffer(GL_FRAMEBUFFER, simulationFramebufferPing);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, positionTexPing, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, velocityTexPing, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, bouncesTexPing, 0);
    glDrawBuffers(3, attachments.data());
    glBindFramebuffer(GL_FRAMEBUFFER, simulationFramebufferPong);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, positionTexPong, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, velocityTexPong, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, bouncesTexPong, 0);
    glDrawBuffers(3, attachments.data());

    // Verify that created framebuffers are complete
    glBindFramebuffer(GL_FRAMEBUFFER, simulationFramebufferPing);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cerr << "Failed to initialise simulation ping framebuffer" << std::endl; }
    glBindFramebuffer(GL_FRAMEBUFFER, simulationFramebufferPong);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cerr << "Failed to initialise simulation pong framebuffer" << std::endl; }
}

void ParticlesSimulator::setInitialData() {
    // Set initial particle data for both framebuffers
    std::array<GLuint, 2> dataFramebuffers = { simulationFramebufferPing, simulationFramebufferPong };
    for (GLuint framebuffer : dataFramebuffers) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        initialDataPass.bind();
        glUniform1ui(initialDataPass.getUniformLocation("numParticles"), config.numParticles);
        glUniform1f(initialDataPass.getUniformLocation("particleRadius"), config.particleRadius);
        glUniform3fv(initialDataPass.getUniformLocation("containerCenter"), 1, glm::value_ptr(config.sphereCenter));
        glUniform1f(initialDataPass.getUniformLocation("containerRadius"), config.sphereRadius);
        utils::renderQuad(initialDataPass);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ParticlesSimulator::deleteFramebuffersAndTextures() {
    // Framebuffers
    glDeleteFramebuffers(1, &simulationFramebufferPing);
    glDeleteFramebuffers(1, &simulationFramebufferPong);

    // Textures
    std::array<GLuint*, 6UL> allTexPtrs = { &positionTexPing, &positionTexPong, &velocityTexPing, &velocityTexPong, &bouncesTexPing, &bouncesTexPong };
    for (GLuint* texPtr : allTexPtrs) { glDeleteTextures(1, texPtr); }
}

void ParticlesSimulator::initShaders() {
    // Simulation shader
    try {
        ShaderBuilder simulationBuilder;
        simulationBuilder.addStage(GL_VERTEX_SHADER,    utils::SHADERS_DIR_PATH / "simulation" / "screen-quad.vert");
        simulationBuilder.addStage(GL_FRAGMENT_SHADER,  utils::SHADERS_DIR_PATH / "simulation" / "particle-sim.frag");
        simulationPass = simulationBuilder.build();
    } catch (ShaderLoadingException e) { std::cerr << e.what() << std::endl; }


    // Draw shader
    try {
        ShaderBuilder drawBuilder;
        drawBuilder.addStage(GL_VERTEX_SHADER,      utils::SHADERS_DIR_PATH / "simulation" / "particle-draw.vert");
        drawBuilder.addStage(GL_FRAGMENT_SHADER,    utils::SHADERS_DIR_PATH / "simulation" / "particle-draw.frag");
        drawPass = drawBuilder.build();
    }  catch (ShaderLoadingException e) { std::cerr << e.what() << std::endl; }

    // Set initial positions and velocities shader
    try {
        ShaderBuilder initialPositionBuilder;
        initialPositionBuilder.addStage(GL_VERTEX_SHADER,   utils::SHADERS_DIR_PATH / "simulation" / "screen-quad.vert");
        initialPositionBuilder.addStage(GL_FRAGMENT_SHADER, utils::SHADERS_DIR_PATH / "simulation" / "particle-set-initial-data.frag");
        initialDataPass = initialPositionBuilder.build();
    }  catch (ShaderLoadingException e) { std::cerr << e.what() << std::endl; }
}

void ParticlesSimulator::simulate() {
    // Figure out which textures to sample from and which framebuffer to draw to
    GLuint drawFramebuffer      = renderToPing ? simulationFramebufferPing : simulationFramebufferPong;
    GLuint samplePositionTex    = renderToPing ? positionTexPong : positionTexPing;
    GLuint sampleVelocityTex    = renderToPing ? velocityTexPong : velocityTexPing;
    GLuint sampleBounceDataTex  = renderToPing ? bouncesTexPong : bouncesTexPing;

    // Bind framebuffer and simulation shader
    glBindFramebuffer(GL_FRAMEBUFFER, drawFramebuffer);
    simulationPass.bind();

    // Bind uniforms (previous iteration textures, timestep, and frame index)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, samplePositionTex);
    glUniform1i(simulationPass.getUniformLocation("previousPositions"), 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, sampleVelocityTex);
    glUniform1i(simulationPass.getUniformLocation("previousVelocities"), 1);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, sampleBounceDataTex);
    glUniform1i(simulationPass.getUniformLocation("previousBounceData"), 2);
    glUniform1f(simulationPass.getUniformLocation("timestep"), config.particleSimTimestep);
    glUniform1ui(simulationPass.getUniformLocation("numParticles"), config.numParticles);
    glUniform1f(simulationPass.getUniformLocation("particleRadius"), config.particleRadius);
    glUniform3fv(simulationPass.getUniformLocation("containerCenter"), 1, glm::value_ptr(config.sphereCenter));
    glUniform1f(simulationPass.getUniformLocation("containerRadius"), config.sphereRadius);
    glUniform1i(simulationPass.getUniformLocation("interParticleCollision"), config.particleInterCollision);

    // Render fullscreen quad to 'touch' all texels
    utils::renderQuad(simulationPass);
}

void ParticlesSimulator::draw(const glm::mat4& viewProjection) {
    // renderToPing indicates which textures the simulation step will render to NEXT
    // This means that we sample from the one that was rendered to LAST, which is !renderToPing
    GLuint samplePositionTex    = renderToPing ? positionTexPong : positionTexPing;
    GLuint sampleVelocityTex    = renderToPing ? velocityTexPong : velocityTexPing;
    GLuint sampleBounceDataTex  = renderToPing ? bouncesTexPong : bouncesTexPing;

    // Bind main framebuffer and drawing shader
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawPass.bind();

    // Bind uniforms
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, samplePositionTex);
    glUniform1i(drawPass.getUniformLocation("positions"), 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, sampleVelocityTex);
    glUniform1i(drawPass.getUniformLocation("velocities"), 1);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, sampleBounceDataTex);
    glUniform1i(drawPass.getUniformLocation("bounceData"), 2);
    glUniformMatrix4fv(drawPass.getUniformLocation("viewProjection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform1ui(drawPass.getUniformLocation("numParticles"), config.numParticles);
    glUniform1f(drawPass.getUniformLocation("particleRadius"), config.particleRadius);
    glUniform3fv(drawPass.getUniformLocation("containerCenter"), 1, glm::value_ptr(config.sphereCenter));
    // ===== Part 2: Drawing =====

    // Render number of instances equal to number of particles
    particleModel.drawInstanced(config.numParticles);
}
