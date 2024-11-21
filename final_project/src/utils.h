#ifndef UTILS_H
#define UTILS_H

#include <framework/shader.h>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <imgui/imgui.h>
#include "Light.h"
#include "curve.h"


static int selectedFolderIndex = -1;

void showDirectoryDropdown(const std::filesystem::path& rootDirectory, std::filesystem::path& selectedDirectoryPath);
void renderLightsIcons(const Light &light, const glm::mat4 &mvpMatrix);

//For Bezier curve movement
glm::vec3 calculatePositionOnBezier(const Curve &curve, const float &t);
std::map<float, float> computeLookupTable(const Curve& curve);
float getTimeFromDistance(const std::map<float, float> &lookup, float distance);

void renderSpacing(const unsigned short spaces, const bool separator);

// for ComplexTerrain class
void initializePlaneVAO(const int res, const int width, GLuint * planeVAO, GLuint * planeVBO, GLuint * planeEBO);

#endif
