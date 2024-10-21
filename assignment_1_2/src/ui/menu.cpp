#include "menu.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <nativefiledialog/nfd.h>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <iostream>

Menu::Menu(Config& config)
: m_config(config)
, m_newParticleCount(config.numParticles)
{}

void Menu::draw() {
    ImGui::Begin("Debug Controls");
    
    ImGui::Text("Particle Simulation");
    ImGui::Separator();
    drawParticleSimControls();
    ImGui::Spacing();
    ImGui::Text("Sphere Container");
    ImGui::Separator();
    drawSphereContainerControls();
    ImGui::Spacing();
    ImGui::Text("Particle Coloring");
    ImGui::Separator();
    drawParticleColorControls();

    ImGui::End();
}

void Menu::drawParticleSimControls() {
    // Parameters
    m_newParticleCount = std::max(1, m_newParticleCount); // Ensure that the new number of particles is always positive
    ImGui::InputInt("New particle count", &m_newParticleCount);
    ImGui::SliderFloat("Timestep", &m_config.particleSimTimestep, 0.001f, 0.05f, "%.3f");
    ImGui::SliderFloat("Particle radius", &m_config.particleRadius, 0.05f, 1.0f);
    ImGui::Checkbox("Inter-particle collisions", &m_config.particleInterCollision);

    // Flags
    std::string simPlaybackText = m_config.doContinuousSimulation ? "Pause simulation" : "Resume simulation";
    if (ImGui::Button(simPlaybackText.c_str())) { m_config.doContinuousSimulation = !m_config.doContinuousSimulation; }
    ImGui::SameLine();
    m_config.doSingleStep = ImGui::Button("Single simulation step");
    ImGui::SameLine();
    if (ImGui::Button("Reset simulation")) {
        m_config.numParticles       = m_newParticleCount;
        m_config.doResetSimulation  = true;
    }
}

void Menu::drawSphereContainerControls() {
    constexpr float CENTER_MAX              = 10.0f;
    constexpr float RADIUS_MAX              = 10.0f;
    constexpr float WIREFRAME_THICKNESS_MAX = 10.0f;

    ImGui::DragFloat3("Center", glm::value_ptr(m_config.sphereCenter), 0.01f, -CENTER_MAX, CENTER_MAX, "%.2f");
    ImGui::DragFloat("Radius", &m_config.sphereRadius, 0.01f, 0.0f, RADIUS_MAX, "%.2f");
    ImGui::ColorEdit3("Color", glm::value_ptr(m_config.sphereColor));
}

void Menu::drawParticleColorControls() {
    ImGui::Checkbox("Use speed-based color", &m_config.useSpeedBasedColor);
    ImGui::BeginDisabled(!m_config.useSpeedBasedColor);
    ImGui::ColorEdit3("Color min speed", glm::value_ptr(m_config.partMinSpeedColor));
    ImGui::SameLine;
    ImGui::ColorEdit3("Color max speed", glm::value_ptr(m_config.partMaxSpeedColor));
    ImGui::DragFloat("Maximum Speed", &m_config.colorMaxSpeed, 0.01f, 0.0f, 20.0f, "%.2f");
    ImGui::EndDisabled();
    ImGui::Checkbox("Use shading", &m_config.useShading);
    ImGui::BeginDisabled(!m_config.useShading);
    ImGui::SliderFloat("Ambient coefficient", &m_config.ambientCoef, 0.0f, 0.5f, "%.2f");
    ImGui::EndDisabled();
    ImGui::Checkbox("Use bounce-based colors", &m_config.useBounceColor);
    ImGui::BeginDisabled(!m_config.useBounceColor);
    ImGui::SliderInt("Bounce threshold", &m_config.bounceThreshold, 1, 50);
    ImGui::SliderInt("Bounce frames", &m_config.bounceFrames, 0, 500);
    ImGui::ColorEdit3("Bounce color", glm::value_ptr(m_config.bounceColor));

    ImGui::EndDisabled();

}

