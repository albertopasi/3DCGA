#include "robot.h"

Robot::Robot():modelMatrices(std::vector<glm::mat4>(12, glm::mat4(1))){
    GPUMesh head = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/head.obj");
    GPUMesh body = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/body.obj");
    GPUMesh leftUpperArm = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/leftUpperArm.obj");
    GPUMesh rightUpperArm = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/rightUpperArm.obj");
    GPUMesh leftForearm = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/leftForearm.obj");
    GPUMesh rightForearm = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/rightForearm.obj");
    GPUMesh leftHand = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/leftHand.obj");
    GPUMesh rightHand = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/rightHand.obj");
    GPUMesh leftThigh = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/leftThigh.obj");
    GPUMesh rightThigh = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/rightThigh.obj");
    GPUMesh leftFoot = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/leftFoot.obj");
    GPUMesh rightFoot = GPUMesh::loadSingleMeshGPU(RESOURCE_ROOT "resources/robot/rightFoot.obj");
    parts.push_back(std::move(head));
    parts.push_back(std::move(body));
    parts.push_back(std::move(leftUpperArm));
    parts.push_back(std::move(rightUpperArm));
    parts.push_back(std::move(leftForearm));
    parts.push_back(std::move(rightForearm));
    parts.push_back(std::move(leftHand));
    parts.push_back(std::move(rightHand));
    parts.push_back(std::move(leftThigh));
    parts.push_back(std::move(rightThigh));
    parts.push_back(std::move(leftFoot));
    parts.push_back(std::move(rightFoot));
    // Center of rotations
    centerVectors.push_back(glm::vec3(-0.004077, 5.41884, -0.13325)); //HEAD
    centerVectors.push_back(glm::vec3(-0.000677, 4.0546, -0.234092)); //BODY 
    centerVectors.push_back(glm::vec3(1.4876, 4.9445, -0.19425)); //LEFT UPPER ARM
    centerVectors.push_back(glm::vec3(-1.4982, 4.9445, -0.19425)); //RIGHT UPPER ARM
    centerVectors.push_back(glm::vec3(1.6959, 4.0795, -0.19425)); //LEFT FOREARM
    centerVectors.push_back(glm::vec3(-1.7065, 4.0795, -0.19425)); //RIGHT FOREARM
    centerVectors.push_back(glm::vec3(1.77529, 3.09506, -0.172538)); //LEFT HAND
    centerVectors.push_back(glm::vec3(-1.78587, 3.09506, -0.172538)); //RIGHT HAND
    centerVectors.push_back(glm::vec3(0.61205, 2.9027, -0.38445)); //LEFT THIGH
    centerVectors.push_back(glm::vec3(-0.62972, 2.9188, -0.34792)); //RIGHT THIGH
    centerVectors.push_back(glm::vec3(0.70019, 1.6238, -0.067447)); //LEFT FOOT
    centerVectors.push_back(glm::vec3(-0.66742, 1.6087, -0.13433)); //RIGHT FOOT
    //rotation limits
    rotationLimits.push_back({-45.0f, 45.0f}); // HEAD
    rotationLimits.push_back({-90.0f, 90.0f}); // BODY
    rotationLimits.push_back({-180.0f, 50.0f}); // LEFT UPPER ARM
    rotationLimits.push_back({-180.0f, 50.0f}); // RIGHT UPPER ARM
    rotationLimits.push_back({-120.0f, 0.0f}); // LEFT FOREARM
    rotationLimits.push_back({-120.0f, 0.0f}); // RIGHT FOREARM
    rotationLimits.push_back({-50.0f, 40.0f}); // LEFT HAND
    rotationLimits.push_back({-50.0f, 40.0f}); // RIGHT HAND
    rotationLimits.push_back({-60.0f, 60.0f}); // LEFT THIGH
    rotationLimits.push_back({-60.0f, 60.0f}); // RIGHT THIGH
    rotationLimits.push_back({-20.0f, 90.0f}); // LEFT FOOT
    rotationLimits.push_back({-20.0f, 90.0f}); // RIGHT FOOT

    currentAngles = std::vector<float>(12, 0.0f);
}

//Calculate rotation matrix
glm::mat4 Robot::calculateRotationMatrix(float angle, glm::vec3 rotationAxis, glm::vec3 rotationCenter){
    glm::mat4 transformation = glm::mat4(1);
    transformation = glm::translate(transformation, rotationCenter);
    transformation = glm::rotate(transformation, glm::radians(angle), rotationAxis);
    transformation = glm::translate(transformation, -rotationCenter);
    return transformation;
}

//Rotate a robot part
void Robot::rotateRobotPart(float angle, glm::vec3 rotationAxis, int part){
    rotateRobotPart(angle, rotationAxis, part, part);
}

void Robot::rotateRobotPart(float angle, glm::vec3 rotationAxis, int part, int center){
    glm::mat4 transf = calculateRotationMatrix(angle, rotationAxis, centerVectors[center]);
    modelMatrices[part] = transf * modelMatrices[part];
    glm::vec4 centerHomog = glm::vec4(centerVectors[part], 1);
    centerVectors[part] = glm::vec3(transf * centerHomog);
}

//Translate the whole robot
void Robot::translate(glm::vec3 translationVector){
    glm::mat4 identity = glm::mat4(1);
    glm::mat4 translation = glm::translate(identity, translationVector);
    // apply translation to every part
    for(auto& model: modelMatrices){
        model = translation * model;
    }
    // apply translation to the rotation centers
    for(auto& center: centerVectors){
        glm::vec4 centerHomog = glm::vec4(center, 1);
        centerHomog = translation * centerHomog;
        center = glm::vec3(centerHomog);
    }
}

//Rotate each part and update facing direction
void Robot::rotate(float angle, glm::vec3 rotationAxis){
    for(int part=0; part<12; part++){
        rotateRobotPart(angle, rotationAxis, part, RobotParts::BODY);
    }
    m_facingDirection = glm::normalize(glm::rotate(glm::mat4(1.0f), glm::radians(angle), rotationAxis) * glm::vec4(m_facingDirection, 0.0f));
}

//Scale the robot
void Robot::scale(glm::vec3 scaling){
    for(auto& model: modelMatrices){
        model = glm::scale(model, scaling);
    }
}

//Rotate head, being sure it's angle is within angle limits
void Robot::rotateHead(float angle){
    float newAngle = currentAngles[RobotParts::HEAD] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::HEAD].minAngle || newAngle > rotationLimits[RobotParts::HEAD].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::HEAD);
    //Update current angle of the part
    currentAngles[RobotParts::HEAD] = newAngle;
}

//Rotate left upper arm, being sure it's angle is within angle limits
void Robot::rotateLeftUpperArm(float angle){
    float newAngle = currentAngles[RobotParts::LEFT_UPPER_ARM] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::LEFT_UPPER_ARM].minAngle || newAngle > rotationLimits[RobotParts::LEFT_UPPER_ARM].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_UPPER_ARM);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_FOREARM, RobotParts::LEFT_UPPER_ARM);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_HAND, RobotParts::LEFT_UPPER_ARM);
    //Update current angle of the part
    currentAngles[RobotParts::LEFT_UPPER_ARM] = newAngle;
}

//Rotate right upper arm, being sure it's angle is within angle limits
void Robot::rotateRightUpperArm(float angle){
    float newAngle = currentAngles[RobotParts::RIGHT_UPPER_ARM] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::RIGHT_UPPER_ARM].minAngle || newAngle > rotationLimits[RobotParts::RIGHT_UPPER_ARM].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_UPPER_ARM);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_FOREARM, RobotParts::RIGHT_UPPER_ARM);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_HAND, RobotParts::RIGHT_UPPER_ARM);
    //Update current angle of the part
    currentAngles[RobotParts::RIGHT_UPPER_ARM] = newAngle;
}

//Rotate left forearm, being sure it's angle is within angle limits
void Robot::rotateLeftForearm(float angle){
    float newAngle = currentAngles[RobotParts::LEFT_FOREARM] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::LEFT_FOREARM].minAngle || newAngle > rotationLimits[RobotParts::LEFT_FOREARM].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_FOREARM);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_HAND, RobotParts::LEFT_FOREARM);
    //Update current angle of the part
    currentAngles[RobotParts::LEFT_FOREARM] = newAngle;
}

//Rotate right forearm, being sure it's angle is within angle limits
void Robot::rotateRightForearm(float angle){
    float newAngle = currentAngles[RobotParts::RIGHT_FOREARM] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::RIGHT_FOREARM].minAngle || newAngle > rotationLimits[RobotParts::RIGHT_FOREARM].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_FOREARM);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_HAND, RobotParts::RIGHT_FOREARM);
    //Update current angle of the part
    currentAngles[RobotParts::RIGHT_FOREARM] = newAngle;
}

//Rotate left hand, being sure it's angle is within angle limits
void Robot::rotateLeftHand(float angle){
    float newAngle = currentAngles[RobotParts::LEFT_HAND] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::LEFT_HAND].minAngle || newAngle > rotationLimits[RobotParts::LEFT_HAND].maxAngle) {
        return;
    }
    //Hierarchical transormations
    rotateRobotPart(angle, m_facingDirection, RobotParts::LEFT_HAND);
    //Update current angle of the part
    currentAngles[RobotParts::LEFT_HAND] = newAngle;
}

//Rotate right hand, being sure it's angle is within angle limits
void Robot::rotateRightHand(float angle){
    float newAngle = currentAngles[RobotParts::RIGHT_HAND] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::RIGHT_HAND].minAngle || newAngle > rotationLimits[RobotParts::RIGHT_HAND].maxAngle) {
        return;
    }
    //Hierarchical transormations
    rotateRobotPart(angle, m_facingDirection, RobotParts::RIGHT_HAND);
    //Update current angle of the part
    currentAngles[RobotParts::RIGHT_HAND] = newAngle;
}

//Rotate left leg, being sure it's angle is within angle limits
void Robot::rotateLeftThigh(float angle){
    float newAngle = currentAngles[RobotParts::LEFT_THIGH] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::LEFT_THIGH].minAngle || newAngle > rotationLimits[RobotParts::LEFT_THIGH].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_THIGH);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_FOOT, RobotParts::LEFT_THIGH);
    //Update current angle of the part
    currentAngles[RobotParts::LEFT_THIGH] = newAngle;
}

//Rotate right leg, being sure it's angle is within angle limits
void Robot::rotateRightThigh(float angle){
    float newAngle = currentAngles[RobotParts::RIGHT_THIGH] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::RIGHT_THIGH].minAngle || newAngle > rotationLimits[RobotParts::RIGHT_THIGH].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_THIGH);
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_FOOT, RobotParts::RIGHT_THIGH);
    //Update current angle of the part
    currentAngles[RobotParts::RIGHT_THIGH] = newAngle;
}

//Rotate left foot, being sure it's angle is within angle limits
void Robot::rotateLeftFoot(float angle){
    float newAngle = currentAngles[RobotParts::LEFT_FOOT] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::LEFT_FOOT].minAngle || newAngle > rotationLimits[RobotParts::LEFT_FOOT].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::LEFT_FOOT);
    //Update current angle of the part
    currentAngles[RobotParts::LEFT_FOOT] = newAngle;
}

//Rotate right foot, being sure it's angle is within angle limits
void Robot::rotateRightFoot(float angle){
    float newAngle = currentAngles[RobotParts::RIGHT_FOOT] + angle;
    // Chech if new angle is within rotation limits
    if (newAngle < rotationLimits[RobotParts::RIGHT_FOOT].minAngle || newAngle > rotationLimits[RobotParts::RIGHT_FOOT].maxAngle) {
        return;
    }
    //Hierarchical transormations
    glm::vec3 perpendicularAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_facingDirection));
    rotateRobotPart(angle, perpendicularAxis, RobotParts::RIGHT_FOOT);
    //Update current angle of the part
    currentAngles[RobotParts::RIGHT_FOOT] = newAngle;
}

//Return the direction the robot is facing, allowing to translate accordingly
glm::vec3 Robot::facingDirection(){
    return m_facingDirection;
}
