#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/trigonometric.hpp>
DISABLE_WARNINGS_POP()

#include <filesystem>
#include <stdint.h>

namespace utils {
    // Camera and screen
    constexpr int32_t WIDTH             = 1280;
    constexpr int32_t HEIGHT            = 720;
    constexpr float FOV                 = glm::radians(90.0f);
    constexpr float ASPECT_RATIO        = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    constexpr glm::vec3 START_POSITION  = {3.0f, 3.0f, 3.0f};
    constexpr glm::vec3 START_LOOK_AT   = -START_POSITION;

    // File paths
    const std::filesystem::path RESOURCES_DIR_PATH  = RESOURCES_DIR;
    const std::filesystem::path SHADERS_DIR_PATH    = SHADERS_DIR;
}
