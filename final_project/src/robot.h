#ifndef ROBOT_H
#define ROBOT_H

#include "mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

enum RobotParts{
    HEAD,
    BODY,
    LEFT_UPPER_ARM,
    RIGHT_UPPER_ARM,
    LEFT_FOREARM,
    RIGHT_FOREARM,
    LEFT_HAND,
    RIGHT_HAND,
    LEFT_THIGH,
    RIGHT_THIGH,
    LEFT_FOOT,
    RIGHT_FOOT
};

struct RotationLimits {
    float minAngle;
    float maxAngle;
};

class Robot{
public:
    Robot();
    void scale(glm::vec3 scaling);

    //Translate the whole robot
    void translate(glm::vec3 translation);

    //Rotate robot and rotate each specific part to allow movement of the robot
    void rotate(float angle, glm::vec3 rotationAxis);
    void rotateHead(float angle);
    void rotateLeftUpperArm(float angle);
    void rotateRightUpperArm(float angle);
    void rotateLeftForearm(float angle);
    void rotateRightForearm(float angle);
    void rotateLeftHand(float angle);
    void rotateRightHand(float angle);
    void rotateLeftThigh(float angle);
    void rotateRightThigh(float angle);
    void rotateLeftFoot(float angle);
    void rotateRightFoot(float angle);

    std::vector<GPUMesh> parts;
    std::vector<glm::mat4> modelMatrices;
    std::vector<glm::vec3> centerVectors;

    //Direction that the robot is facing
    glm::vec3 facingDirection();
    glm::mat4 getGlobalRotation();
private:
    glm::mat4 calculateRotationMatrix(float angle, glm::vec3 rotationAxis, glm::vec3 rotationCenter);
    void rotateRobotPart(float angle, glm::vec3 rotationAxis, int part);
    void rotateRobotPart(float angle, glm::vec3 rotationAxis, int part, int center);

    // Robot initially faces positive Z-axis
    glm::vec3 m_facingDirection = glm::vec3(0.0f, 0.0f, 1.0f);

    //Rotation limit to limit movement of the robot's parts
    std::vector<RotationLimits> rotationLimits;
    std::vector<float> currentAngles;
    glm::mat4 m_globalRotation = glm::mat4(1.0f);
};

#endif