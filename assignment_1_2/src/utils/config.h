#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()


struct Config {
    // Particle simulation parameters
    uint32_t numParticles       = 25;
    float particleSimTimestep   = 0.014f;
    float particleRadius        = 0.45f;
    bool particleInterCollision = true;

    // Particle simulation flags
    bool doSingleStep           = false;
    bool doContinuousSimulation = true;
    bool doResetSimulation      = false;

    // Container sphere parameters
    glm::vec3 sphereCenter          = glm::vec3(0.0f);
    float sphereRadius              = 3.0f;
    glm::vec3 sphereColor           = glm::vec3(1.0f);

    // ===== Part 2: Drawing =====
    bool useSpeedBasedColor         = false;
    glm::vec3 partMinSpeedColor     = glm::vec3(0,0,1);
    glm::vec3 partMaxSpeedColor     = glm::vec3(1,0,0);
    float colorMaxSpeed             = 7.5f;
    bool useShading                 = false;
    float ambientCoef               = 0.25;
};
