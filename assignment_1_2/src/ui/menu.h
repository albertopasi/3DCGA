#pragma once

#include <utils/config.h>


class Menu {
public:
    Menu(Config& config);

    void draw();

private:
    void drawParticleSimControls();
    void drawSphereContainerControls();
    void drawParticleColorControls();

    Config& m_config;
    int32_t m_newParticleCount;
};
