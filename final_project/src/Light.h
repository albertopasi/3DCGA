#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include "texture.h"

struct Light {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 direction;
    bool is_spotlight;
    ShadowMap shadowMap;
};

#endif
