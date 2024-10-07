#pragma once

#include <framework/shader.h>

#include <render/mesh.h>
#include <utils/config.h>


class SphereContainer {
public:
    SphereContainer(const Config& config);

    void draw(const glm::mat4 viewProjection);
private:
    const Config& config;
    
    GPUMesh model;
    Shader drawSpherePass;
};
