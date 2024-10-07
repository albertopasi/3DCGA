#include "sphere_container.h"

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()

#include <utils/constants.h>

#include <iostream>


SphereContainer::SphereContainer(const Config& config)
    : config(config)
    , model(utils::RESOURCES_DIR_PATH / "sphere.obj", true) {
    try {
        ShaderBuilder sphereDrawBuilder;
        sphereDrawBuilder.addStage(GL_VERTEX_SHADER,    utils::SHADERS_DIR_PATH / "sphere-container" / "draw-sphere.vert");
        sphereDrawBuilder.addStage(GL_FRAGMENT_SHADER,  utils::SHADERS_DIR_PATH / "sphere-container" / "draw-sphere.frag");
        drawSpherePass = sphereDrawBuilder.build();
    } catch (ShaderLoadingException e) { std::cerr << e.what() << std::endl; }

}

void SphereContainer::draw(const glm::mat4 viewProjection) {
    // Toggle on wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Bind main framebuffer and drawing shader
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawSpherePass.bind();

    // Set uniforms and draw
    glUniformMatrix4fv(drawSpherePass.getUniformLocation("viewProjection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform3fv(drawSpherePass.getUniformLocation("center"), 1, glm::value_ptr(config.sphereCenter));
    glUniform1f(drawSpherePass.getUniformLocation("radius"), config.sphereRadius);
    glUniform3fv(drawSpherePass.getUniformLocation("color"), 1, glm::value_ptr(config.sphereColor));
    model.draw();

    // Toggle wireframe back off
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
