//#include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>

#include "terrain.h"
#include "particle_system.h"

DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include <chrono>

#include "framework/trackball.h"
#include "utils.h"
#include "Camera.h"
#include "Light.h"
#include "curve.h"
#include "point.h"
#include "robot.h"
#include "minimap.h"

enum class DisplayModeType {
    SCENE1,
    SCENE2,
    SCENE3,
};
DisplayModeType DISPLAY_MODE = DisplayModeType::SCENE1;

constexpr glm::vec3 START_POSITION  = {5.0f, 5.0f, 5.0f};
constexpr glm::vec3 START_TARGET  = {-0.004077, 4.41884, -0.13325}; //just below robot head centre of rotation
constexpr glm::vec3 START_LOOK_AT   = START_TARGET-START_POSITION;

constexpr glm::vec3 START_POS_THIRD_VIEW  = {-0.000677, 8.0546, -6.234092};
constexpr glm::vec3 START_TARGET_THIRD_VIEW  = {-0.000677, 4.0546, -0.234092}; //robot body centre of rotation
constexpr glm::vec3 START_LOOK_AT_THIRD_VIEW = START_TARGET_THIRD_VIEW - START_POS_THIRD_VIEW;

constexpr glm::vec3 START_POS_BIRD_VIEW  = {-0.000677, 20, -0.234092};
constexpr glm::vec3 START_TARGET_BIRD_VIEW  = {-0.000677, 4.0546, -0.134092}; //robot body centre of rotation
constexpr glm::vec3 START_LOOK_AT_BIRD_VIEW = START_TARGET_BIRD_VIEW - START_POS_BIRD_VIEW;


const std::filesystem::path PBR_TEXTURES_FOLDER         = RESOURCE_ROOT "resources/textures";
const std::filesystem::path PBR_TEXTURES_FOLDER_DEFAULT = RESOURCE_ROOT "resources/textures/wood";

int MAX_LIGHTS = GL_MAX_LIGHTS;
const float FULL_POINT_ANIMATION = 8.0;
int WINDOW_SIZE = 1024;

float getDeltaTime() {
    using namespace std::chrono;
    static steady_clock::time_point lastFrameTime = steady_clock::now();
    steady_clock::time_point currentFrameTime = steady_clock::now();
    duration<float, std::milli> deltaTime = duration_cast<duration<float, std::milli>>(currentFrameTime - lastFrameTime);
    lastFrameTime = currentFrameTime;
    return deltaTime.count();
}

class Application {
public:
    Application():  m_window("Final Project", glm::ivec2(WINDOW_SIZE, WINDOW_SIZE), OpenGLVersion::GL41),
                    m_texture(RESOURCE_ROOT "resources/checkerboard.png"),
                    m_pbrTexturesDirectory(PBR_TEXTURES_FOLDER_DEFAULT),
                    m_pbrTextures(PBR_TEXTURES_FOLDER_DEFAULT),
                    m_cubemap(RESOURCE_ROOT "resources/textures/cubemap"),
                    //m_trackball(&m_window, glm::radians(45.0f), 20.0f, 0.0f, 0.0f),
                    m_camera(&m_window, START_POSITION, START_LOOK_AT, START_TARGET),
                    m_camera_third_person(&m_window, START_POS_THIRD_VIEW, START_LOOK_AT_THIRD_VIEW, START_TARGET),
                    m_camera_birds_eye(&m_window, START_POS_BIRD_VIEW, START_LOOK_AT_BIRD_VIEW, START_TARGET_BIRD_VIEW),
                    m_terrain(50, 100),
                    m_minimap(),
                    m_light(Light{
                       START_POSITION,
                       glm::vec3(1.0),
                       START_LOOK_AT,
                       false,
                       ShadowMap()
                    }),
                    m_light1(Light{
                       glm::vec3(-2.4, 1.2, 0.2),
                       glm::vec3(1.0),
                       glm::vec3(0.0, 0.0, 0.0),
                       false,
                       ShadowMap()
                   }),
                    fire(),
                    m_robot(),
                    m_robotTranslateVector(glm::vec3(0)),
                    m_robotRotationAngle(10.0),
                    m_robotRotationAxis(glm::vec3(1.0, 0.0, 0.0)),
                    m_bezier( Curve{
                        {
                            glm::vec3(-0.95f,  0.05f, 0.0f), 
                            glm::vec3(-0.85f,  2.31f, -1.48f), 
                            glm::vec3( 0.50f, 0.05f, 0.37f), 
                            glm::vec3( 0.00f,  1.92f, 1.48f),
                        },
                        glm::vec4(1.0, 0.0, 0.0, 1.0)
                    }),
                    m_bezier1( Curve{
                        {
                            glm::vec3(0.00f,  1.92f, 1.48f), 
                            glm::vec3(1.92f, 2.31f, 4.81f), 
                            glm::vec3(0.50f, -1.15f, 0.37f), 
                            glm::vec3(1.54f, 1.92f, 1.48f),
                        },
                        glm::vec4(1.0, 0.0, 0.0, 1.0)
                    }),
                    m_bezier2( Curve{
                        {
                            glm::vec3(1.54f, 1.92f, 1.48f), 
                            glm::vec3(3.08f, 3.08f, 0.37f), 
                            glm::vec3(0.50f, -1.15f, 0.74f), 
                            glm::vec3(1.54f, 1.92f, -0.74f),
                        },
                        glm::vec4(1.0, 0.0, 0.0, 1.0)
                    }),
                    m_curve(m_bezier),
                    m_curve1(m_bezier1),
                    m_curve2(m_bezier2)
                   {
        
        fire.InitParticleSystem(glm::vec3(0.0f, 1.0f, 0.0f));
        m_camera.setUserInteraction(true);
        m_camera_third_person.setUserInteraction(false);
        m_camera_birds_eye.setUserInteraction(false);
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });


        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/scene.obj");
        m_bezierLookup = computeLookupTable(m_bezier);
        std::cout << "Curve length = " << m_bezierLookup[3.07679] << std::endl;
        m_meshes_cube = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/cubemap.obj");

        // Check framebuffer status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Minimap framebuffer not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer after setup

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder pbrBuilder;
            pbrBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            pbrBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/pbr_frag.glsl");
            m_pbrShader = pbrBuilder.build();

            ShaderBuilder bezierBuilder;
            bezierBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/bezier_vert.glsl");
            bezierBuilder.addStage(GL_TESS_CONTROL_SHADER, RESOURCE_ROOT "shaders/bezier_tess.glsl");
            bezierBuilder.addStage(GL_TESS_EVALUATION_SHADER, RESOURCE_ROOT "shaders/bezier_tess_eval.glsl");
            bezierBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bezier_frag.glsl");
            m_bezierShader = bezierBuilder.build();

            ShaderBuilder pointOnBezierBuilder;
            pointOnBezierBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/point_on_bezier_vert.glsl");
            pointOnBezierBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/point_on_bezier_frag.glsl");
            m_pointOnBezierShader = pointOnBezierBuilder.build();

            ShaderBuilder environmentMappingBuilder;
            environmentMappingBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/cubemap_vert.glsl");
            environmentMappingBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/cubemap_frag.glsl");
            m_environmentMappingShader = environmentMappingBuilder.build();

            ShaderBuilder reflectionBuilder;
            reflectionBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/reflection_vert.glsl");
            reflectionBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/reflection_frag.glsl");
            m_reflectionShader = reflectionBuilder.build();

            ShaderBuilder refractionBuilder;
            refractionBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/reflection_vert.glsl"); //same as reflection
            refractionBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/refraction_frag.glsl");
            m_refractionShader = refractionBuilder.build();

            ShaderBuilder lambertBuilder;
            lambertBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            lambertBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lambert_frag.glsl");
            m_lambertShader= lambertBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }
    

    void update()
    {

        int dummyInteger = 0; // Initialized to 0
        float distanceTravelled = 0.0;
        const float CONSTANT_DISTANCE = m_bezierLookup[1.0];

        //Minimap framebuffer
        GLuint framebuffer, minimapTexture, minimapRenderbuffer;
        // Create and bind the framebuffer
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Generate and bind the texture for the minimap
        glGenTextures(1, &minimapTexture);
        glBindTexture(GL_TEXTURE_2D, minimapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Linear filter for minification
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Linear filter for magnification
        glBindTexture(GL_TEXTURE_2D, 0);

        // Attach texture to framebuffer as color attachment
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, minimapTexture, 0);

        // Create and bind the renderbuffer for depth/stencil
        glGenRenderbuffers(1, &minimapRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, minimapRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1024, 1024); // Depth and stencil attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, minimapRenderbuffer);

        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);

        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            float deltaTime = getDeltaTime();

            m_window.updateInput();
            //m_camera.updateInput(currentViewMode);
            if (currentViewMode == DEFAULT) {
                m_camera.updateInput(currentViewMode);
                m_viewMatrix = m_camera.viewMatrix();
            }
            else if (currentViewMode == THIRD_PERSON_VIEW) {
                m_viewMatrix = m_camera_third_person.viewMatrix();
            }
            else if (currentViewMode == BIRDS_EYE_VIEW) {
                m_viewMatrix = m_camera_birds_eye.viewMatrix();
            }


            checkWireframe();

            // Use ImGui for easy input/output of ints, floats, strings, etc...
            ImGui::Begin("Window");

            // Declare display modes and names
            std::array displayModeNames { "SCENE 1", "SCENE 2", "SCENE 3"};
            const std::array displayModes {
                DisplayModeType::SCENE1,
                DisplayModeType::SCENE2,
                DisplayModeType::SCENE3,
            };
            // get the index of the current display mode, as current mode
            int current_mode = static_cast<int>(DISPLAY_MODE);
            // update current mode based on menu
            ImGui::Combo("Display Mode", &current_mode, displayModeNames.data(), displayModeNames.size());
            // set display mode
            DISPLAY_MODE = displayModes[current_mode];

            ImGui::Checkbox("Use material if no texture", &m_useMaterial);
            ImGui::Checkbox("Wireframe mode", &m_wireframe);
            ImGui::Checkbox("Activate Default Shading", &m_useDefaultShading);
            ImGui::Checkbox("Constant speed on path", &m_constantSpeedOnPath);
            ImGui::SliderFloat3("Control point 0", glm::value_ptr(m_bezier.controlPoints[0]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 1", glm::value_ptr(m_bezier.controlPoints[1]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 2", glm::value_ptr(m_bezier.controlPoints[2]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 3", glm::value_ptr(m_bezier.controlPoints[3]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 4", glm::value_ptr(m_bezier1.controlPoints[0]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 5", glm::value_ptr(m_bezier1.controlPoints[1]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 6", glm::value_ptr(m_bezier1.controlPoints[2]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 7", glm::value_ptr(m_bezier1.controlPoints[3]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 8", glm::value_ptr(m_bezier2.controlPoints[0]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 9", glm::value_ptr(m_bezier2.controlPoints[1]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 10", glm::value_ptr(m_bezier2.controlPoints[2]), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat3("Control point 11", glm::value_ptr(m_bezier2.controlPoints[3]), -10.0f, 10.0f, "%.2f");
          
            // --------------------- ROBOT CONTROLS START
            ImGui::SliderFloat("Rotate robot angle", &m_robotRotationAngle, -90.0f, 90.0f, "%.2f");
            // ----------------------- ROBOT CONTROLS END
          
            if (ImGui::Checkbox("Use Environment Mapping", &m_useEnvironmentMapping)) {
                // Se viene deselezionato, forzare l'uncheck degli altri due checkbox
                if (!m_useEnvironmentMapping) {
                    m_useReflectionShading = false;
                    m_useRefractionShading = false;
                }
            }
            ImGui::BeginDisabled(!m_useEnvironmentMapping);
            ImGui::Checkbox("Activate Reflection", &m_useReflectionShading);
            ImGui::Checkbox("Activate Refraction", &m_useRefractionShading);
            ImGui::EndDisabled();
          
            ImGui::Checkbox("Activate normal map", &m_useNormalMapping);
            renderSpacing(2, true);

            if(ImGui::CollapsingHeader("PBR Controls", ImGuiTreeNodeFlags_DefaultOpen)) { renderSpacing(2, false);
                ImGui::Checkbox("Activate PBR Shading", &m_usePBRShading); renderSpacing(2, false);

                showDirectoryDropdown(PBR_TEXTURES_FOLDER, m_pbrTexturesDirectory);
            }

            ImGui::Checkbox("Activate lambert", &m_lambert);
            ImGui::Checkbox("shadows", &m_shadow);

            renderSpacing(2, true);

            if (ImGui::CollapsingHeader("Light Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
                renderSpacing(2, false);
                ImGui::ColorEdit3("Light Color", glm::value_ptr(m_light.color));
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Light color"); renderSpacing(2, false);
                ImGui::Text("Light Position"); renderSpacing(2, false);
                ImGui::SliderFloat3("light pos", glm::value_ptr(m_light.position), -10.0f, 10.0f, "%.2f");

                ImGui::ColorEdit3("Light 2 Color", glm::value_ptr(m_light1.color));
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Light 2 color"); renderSpacing(2, false);
                ImGui::Text("Light 2 Position"); renderSpacing(2, false);
                ImGui::SliderFloat3("light 2 pos", glm::value_ptr(m_light1.position), -10.0f, 10.0f, "%.2f");
            }
            
            
            ImGui::End();

            ImGui::Begin("Camera Views");
            if (ImGui::Button("Default View Mode")) {
                currentViewMode = DEFAULT;
            }
            if (ImGui::Button("Third-Person View Mode")) {
                currentViewMode = THIRD_PERSON_VIEW;
            }
            if (ImGui::Button("Bird's Eye View Mode")) {
                currentViewMode = BIRDS_EYE_VIEW;
            }
            
            ImGui::End();

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);

            const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
            const glm::mat4 vpMatrix = m_projectionMatrix * m_viewMatrix;

            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

            if(current_mode==0){

                for(int i = 0; i < 12; i++){
                    const glm::mat3 normalModelMatrixRobot = glm::inverseTranspose(glm::mat3(m_robot.modelMatrices[i]));

                    if(m_useDefaultShading) {
                        m_defaultShader.bind();
                        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(vpMatrix * m_robot.modelMatrices[i]));
                        //Uncomment this line when you use the modelMatrix (or fragmentPosition)
                        //glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        //glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        if (m_robot.parts[i].hasTextureCoords()) {
                            m_texture.bind(GL_TEXTURE0);
                            glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
                        } else {
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                        }
                        m_robot.parts[i].draw(m_defaultShader);
                    }
                    else if(m_usePBRShading) {
                        m_pbrTextures.load(m_pbrTexturesDirectory);
                        
                        m_pbrShader.bind();
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(vpMatrix * m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix3fv(m_pbrShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));

                    // PBR texture binding
                        m_pbrTextures.getTexture("albedo").bind(GL_TEXTURE1);
                        //m_texture.bind(GL_TEXTURE1);
                        glUniform1i(m_pbrShader.getUniformLocation("albedoMap"), 1);
                        m_pbrTextures.getTexture("roughness").bind(GL_TEXTURE2);
                        glUniform1i(m_pbrShader.getUniformLocation("roughnessMap"), 2);
                        m_pbrTextures.getTexture("ao").bind(GL_TEXTURE3);
                        glUniform1i(m_pbrShader.getUniformLocation("aoMap"), 3);
                        if(m_pbrTextures.isMetallic) {
                            m_pbrTextures.getTexture("metallic").bind(GL_TEXTURE4);
                            glUniform1i(m_pbrShader.getUniformLocation("metallicMap"), 4);
                        }
                        m_pbrTextures.getTexture("normal").bind(GL_TEXTURE5);
                        glUniform1i(m_pbrShader.getUniformLocation("normalMap"), 5);
                        glUniform1i(m_pbrShader.getUniformLocation("enableNormalMapping"), m_useNormalMapping);
                        
                        glUniform3fv(m_pbrShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        glUniform3fv(m_pbrShader.getUniformLocation("lightPos"), 1, glm::value_ptr(m_light.position));
                        glUniform3fv(m_pbrShader.getUniformLocation("lightColor"), 1, glm::value_ptr(m_light.color));
                        glUniform1i(m_pbrShader.getUniformLocation("isMetallic"), m_pbrTextures.isMetallic);

                        m_robot.parts[i].draw(m_pbrShader);
                    }else if(m_useReflectionShading) {
                        m_reflectionShader.bind();
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        glUniformMatrix3fv(m_reflectionShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        glUniform3fv(m_reflectionShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_reflectionShader.getUniformLocation("cubemap"), 6);

                        m_robot.parts[i].draw(m_reflectionShader);
                    }
                    else if(m_useRefractionShading) {
                        m_refractionShader.bind();
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        glUniformMatrix3fv(m_refractionShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        glUniform3fv(m_refractionShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_refractionShader.getUniformLocation("cubemap"), 6);

                        m_robot.parts[i].draw(m_refractionShader);
                    }
                }
                
                glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
                if(m_useEnvironmentMapping) {
                    for(GPUMesh& mesh : m_meshes_cube){
                        m_environmentMappingShader.bind();
                        glm::mat4 noTranslationViewMatrix = glm::mat4(glm::mat3(m_viewMatrix));
                        glUniformMatrix4fv(m_environmentMappingShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(noTranslationViewMatrix));
                        glUniformMatrix4fv(m_environmentMappingShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_environmentMappingShader.getUniformLocation("cubemap"), 6);

                        mesh.draw(m_environmentMappingShader);

                    }
                }
                glDepthFunc(GL_LESS); // set depth function back to default                
            
                //Render to texture for minimap 
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);     
                glViewport(0,0,1024,1024);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear the framebuffer
            
                // Render the terrain
                glm::mat4 m_viewMatrix_minimap = m_camera_birds_eye.viewMatrix();
                const glm::mat4 mvpMatrix_minimap = m_projectionMatrix * m_viewMatrix_minimap * m_modelMatrix;
                const glm::mat4 vpMatrix_minimap = m_projectionMatrix * m_viewMatrix_minimap;

                for(int i = 0; i < 12; i++){
                    const glm::mat3 normalModelMatrixRobot = glm::inverseTranspose(glm::mat3(m_robot.modelMatrices[i]));

                    if(m_useDefaultShading) {
                        m_defaultShader.bind();
                        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(vpMatrix_minimap * m_robot.modelMatrices[i]));
                        //Uncomment this line when you use the modelMatrix (or fragmentPosition)
                        //glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        //glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        if (m_robot.parts[i].hasTextureCoords()) {
                            m_texture.bind(GL_TEXTURE0);
                            glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
                        } else {
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                        }
                        m_robot.parts[i].draw(m_defaultShader);
                    }
                    else if(m_usePBRShading) {
                        m_pbrTextures.load(m_pbrTexturesDirectory);
                        
                        m_pbrShader.bind();
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(vpMatrix_minimap * m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix3fv(m_pbrShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));

                    // PBR texture binding
                        m_pbrTextures.getTexture("albedo").bind(GL_TEXTURE1);
                        //m_texture.bind(GL_TEXTURE1);
                        glUniform1i(m_pbrShader.getUniformLocation("albedoMap"), 1);
                        m_pbrTextures.getTexture("roughness").bind(GL_TEXTURE2);
                        glUniform1i(m_pbrShader.getUniformLocation("roughnessMap"), 2);
                        m_pbrTextures.getTexture("ao").bind(GL_TEXTURE3);
                        glUniform1i(m_pbrShader.getUniformLocation("aoMap"), 3);
                        if(m_pbrTextures.isMetallic) {
                            m_pbrTextures.getTexture("metallic").bind(GL_TEXTURE4);
                            glUniform1i(m_pbrShader.getUniformLocation("metallicMap"), 4);
                        }
                        m_pbrTextures.getTexture("normal").bind(GL_TEXTURE5);
                        glUniform1i(m_pbrShader.getUniformLocation("normalMap"), 5);
                        glUniform1i(m_pbrShader.getUniformLocation("enableNormalMapping"), m_useNormalMapping);
                        
                        glUniform3fv(m_pbrShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        glUniform3fv(m_pbrShader.getUniformLocation("lightPos"), 1, glm::value_ptr(m_light.position));
                        glUniform3fv(m_pbrShader.getUniformLocation("lightColor"), 1, glm::value_ptr(m_light.color));
                        glUniform1i(m_pbrShader.getUniformLocation("isMetallic"), m_pbrTextures.isMetallic);

                        m_robot.parts[i].draw(m_pbrShader);
                    }else if(m_useReflectionShading) {
                        m_reflectionShader.bind();
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix_minimap));
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        glUniformMatrix3fv(m_reflectionShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        glUniform3fv(m_reflectionShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_reflectionShader.getUniformLocation("cubemap"), 6);

                        m_robot.parts[i].draw(m_reflectionShader);
                    }
                    else if(m_useRefractionShading) {
                        m_refractionShader.bind();
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix_minimap));
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        glUniformMatrix3fv(m_refractionShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        glUniform3fv(m_refractionShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_refractionShader.getUniformLocation("cubemap"), 6);

                        m_robot.parts[i].draw(m_refractionShader);
                    }
                }
            
                glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
                if(m_useEnvironmentMapping) {
                    for(GPUMesh& mesh : m_meshes_cube){
                        m_environmentMappingShader.bind();
                        glm::mat4 noTranslationViewMatrix = glm::mat4(glm::mat3(m_viewMatrix_minimap));
                        glUniformMatrix4fv(m_environmentMappingShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(noTranslationViewMatrix));
                        glUniformMatrix4fv(m_environmentMappingShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_environmentMappingShader.getUniformLocation("cubemap"), 6);

                        mesh.draw(m_environmentMappingShader);

                    }
                }
                glDepthFunc(GL_LESS); // set depth function back to default   

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glViewport(0,0, WINDOW_SIZE, WINDOW_SIZE);
                if(currentViewMode!=2){
                    m_minimap.drawMinimap(minimapTexture);
                }
            }
            else if(current_mode ==1){
                float terrainHeight = m_terrain.heightAtPos(m_camera.cameraPos()[0], m_camera.cameraPos()[2]);
                m_camera.setCameraPos(glm::vec3(m_camera.cameraPos()[0], terrainHeight + 2.0, m_camera.cameraPos()[2]));

                //Render the terrain
                m_terrain.draw(mvpMatrix, m_viewMatrix, m_camera.cameraPos(), m_light.position);         


                //Render to texture for minimap 
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);     
                glViewport(0,0,1024,1024);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear the framebuffer
            
                // Render the terrain
                glm::mat4 m_viewMatrix_minimap = m_camera_birds_eye.viewMatrix();
                const glm::mat4 mvpMatrix_minimap = m_projectionMatrix * m_viewMatrix_minimap * m_modelMatrix;
                const glm::mat4 vpMatrix_minimap = m_projectionMatrix * m_viewMatrix_minimap;

                for(int i = 0; i < 12; i++){
                    const glm::mat3 normalModelMatrixRobot = glm::inverseTranspose(glm::mat3(m_robot.modelMatrices[i]));

                    if(m_useDefaultShading) {
                        m_defaultShader.bind();
                        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(vpMatrix_minimap * m_robot.modelMatrices[i]));
                        if (m_robot.parts[i].hasTextureCoords()) {
                            m_texture.bind(GL_TEXTURE0);
                            glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
                        } else {
                            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                        }
                        m_robot.parts[i].draw(m_defaultShader);
                    }
                    else if(m_usePBRShading) {
                        m_pbrTextures.load(m_pbrTexturesDirectory);
                        
                        m_pbrShader.bind();
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(vpMatrix_minimap * m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_pbrShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix3fv(m_pbrShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));

                    // PBR texture binding
                        m_pbrTextures.getTexture("albedo").bind(GL_TEXTURE1);
                        //m_texture.bind(GL_TEXTURE1);
                        glUniform1i(m_pbrShader.getUniformLocation("albedoMap"), 1);
                        m_pbrTextures.getTexture("roughness").bind(GL_TEXTURE2);
                        glUniform1i(m_pbrShader.getUniformLocation("roughnessMap"), 2);
                        m_pbrTextures.getTexture("ao").bind(GL_TEXTURE3);
                        glUniform1i(m_pbrShader.getUniformLocation("aoMap"), 3);
                        if(m_pbrTextures.isMetallic) {
                            m_pbrTextures.getTexture("metallic").bind(GL_TEXTURE4);
                            glUniform1i(m_pbrShader.getUniformLocation("metallicMap"), 4);
                        }
                        m_pbrTextures.getTexture("normal").bind(GL_TEXTURE5);
                        glUniform1i(m_pbrShader.getUniformLocation("normalMap"), 5);
                        glUniform1i(m_pbrShader.getUniformLocation("enableNormalMapping"), m_useNormalMapping);
                        
                        glUniform3fv(m_pbrShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        glUniform3fv(m_pbrShader.getUniformLocation("lightPos"), 1, glm::value_ptr(m_light.position));
                        glUniform3fv(m_pbrShader.getUniformLocation("lightColor"), 1, glm::value_ptr(m_light.color));
                        glUniform1i(m_pbrShader.getUniformLocation("isMetallic"), m_pbrTextures.isMetallic);

                        m_robot.parts[i].draw(m_pbrShader);
                    }else if(m_useReflectionShading) {
                        m_reflectionShader.bind();
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix_minimap));
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_reflectionShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        glUniformMatrix3fv(m_reflectionShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        glUniform3fv(m_reflectionShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_reflectionShader.getUniformLocation("cubemap"), 6);

                        m_robot.parts[i].draw(m_reflectionShader);
                    }
                    else if(m_useRefractionShading) {
                        m_refractionShader.bind();
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix_minimap));
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_robot.modelMatrices[i]));
                        glUniformMatrix4fv(m_refractionShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        glUniformMatrix3fv(m_refractionShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrixRobot));
                        glUniform3fv(m_refractionShader.getUniformLocation("camPos"), 1, glm::value_ptr(m_camera.cameraPos()));
                        
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_refractionShader.getUniformLocation("cubemap"), 6);

                        m_robot.parts[i].draw(m_refractionShader);
                    }
                }
                m_terrain.draw(mvpMatrix_minimap, m_viewMatrix_minimap, m_camera_birds_eye.cameraPos(), m_light.position);
            
                glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
                if(m_useEnvironmentMapping) {
                    for(GPUMesh& mesh : m_meshes_cube){
                        m_environmentMappingShader.bind();
                        glm::mat4 noTranslationViewMatrix = glm::mat4(glm::mat3(m_viewMatrix_minimap));
                        glUniformMatrix4fv(m_environmentMappingShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(noTranslationViewMatrix));
                        glUniformMatrix4fv(m_environmentMappingShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                        m_cubemap.bind(GL_TEXTURE6);
                        glUniform1i(m_environmentMappingShader.getUniformLocation("cubemap"), 6);

                        mesh.draw(m_environmentMappingShader);

                    }
                }
                glDepthFunc(GL_LESS); // set depth function back to default   

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glViewport(0,0, WINDOW_SIZE, WINDOW_SIZE);
                if(currentViewMode!=2){
                    m_minimap.drawMinimap(minimapTexture);
                }
            }
            else if(current_mode==2){
                fire.Render(int(deltaTime), vpMatrix, m_camera.cameraPos());

                // Draw bezier curve
                {
                    m_curve = GPUCurve(m_bezier);
                    m_bezierShader.bind();
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                    glUniform1f(m_bezierShader.getUniformLocation("segmentCount"), 40.0);
                    glUniform4fv(m_bezierShader.getUniformLocation("tint"), 1, glm::value_ptr(m_bezier.color));
                    m_curve.draw(m_bezierShader);
                }
                
                {
                    m_curve1 = GPUCurve(m_bezier1);
                    m_bezierShader.bind();
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                    glUniform1f(m_bezierShader.getUniformLocation("segmentCount"), 40.0);
                    glUniform4fv(m_bezierShader.getUniformLocation("tint"), 1, glm::value_ptr(m_bezier1.color));
                    m_curve1.draw(m_bezierShader);
                }

                {
                    m_curve2 = GPUCurve(m_bezier2);
                    m_bezierShader.bind();
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
                    glUniformMatrix4fv(m_bezierShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                    glUniform1f(m_bezierShader.getUniformLocation("segmentCount"), 40.0);
                    glUniform4fv(m_bezierShader.getUniformLocation("tint"), 1, glm::value_ptr(m_bezier1.color));
                    m_curve2.draw(m_bezierShader);
                }
                
                float time = glfwGetTime();

                // toggle non-constant/constant speed
                {
                    float t = 0.0;
                    if(m_constantSpeedOnPath){
                        distanceTravelled = CONSTANT_DISTANCE * (time - (FULL_POINT_ANIMATION * int(time/FULL_POINT_ANIMATION)))  / FULL_POINT_ANIMATION;
                        t = getTimeFromDistance(m_bezierLookup, distanceTravelled);
                    }else{
                    t = (time - (FULL_POINT_ANIMATION * int(time/FULL_POINT_ANIMATION)))  / FULL_POINT_ANIMATION;
                    }
                    if((int)time % int(FULL_POINT_ANIMATION*3) < FULL_POINT_ANIMATION){
                        glm::vec3 p = calculatePositionOnBezier(m_bezier, t);
                        m_light1.position = p;
                    }else if((int)time % int(FULL_POINT_ANIMATION*3) < 2*FULL_POINT_ANIMATION){
                        glm::vec3 p = calculatePositionOnBezier(m_bezier1, t);
                        m_light1.position = p;
                    }else{
                        glm::vec3 p = calculatePositionOnBezier(m_bezier2, t);
                        m_light1.position = p;
                    }
                }

                    //Calculate lightMVP
                    float nearPlane = 1.0f;
                    float farPlane = 100.0f;
                    constexpr float fov = glm::pi<float>() / 4.0f;
                    const float aspectRatio = static_cast<float>(m_window.getWindowSize().x) / static_cast<float>(m_window.getWindowSize().y);
                    glm::mat4 lightProjectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
                    glm::vec3 lightPosition = m_light.position;
                    glm::vec3 lightTarget = m_light.direction - lightPosition;
                    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
                    glm::vec3 upVector = up;
                    if(lightTarget != glm::vec3(0.0, 0.0, 0.0)){
                        lightTarget = glm::normalize(lightTarget);
                        glm::vec3 rightVector = glm::normalize(glm::cross(up, lightTarget));
                        upVector = glm::cross(lightTarget, rightVector);
                    }
                    glm::mat4 lightView = glm::lookAt(lightPosition, lightTarget, upVector);
                    glm::mat4 modelMatrix = m_modelMatrix;  
                    glm::mat4 lightMVP = lightProjectionMatrix * lightView * modelMatrix;

                    m_light.shadowMap.bindFramebuffer();
                    for(GPUMesh& mesh: m_meshes){
                        m_shadowShader.bind();
                        glUniformMatrix4fv(m_shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                        mesh.draw(m_shadowShader);
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);

                    glm::vec3 lightPosition1 = m_light1.position;
                    glm::vec3 lightTarget1 = m_light1.direction - lightPosition1;
                    upVector = up;
                    if(lightTarget1 != glm::vec3(0.0, 0.0, 0.0)){
                        lightTarget1 = glm::normalize(lightTarget1);
                        glm::vec3 rightVector = glm::normalize(glm::cross(up, lightTarget1));
                        upVector = glm::cross(lightTarget1, rightVector);
                    }
                    glm::mat4 lightView1 = glm::lookAt(lightPosition1, lightTarget1, upVector);  
                    glm::mat4 modelMatrix1 = glm::mat4(1.0f);  
                    glm::mat4 lightMVP1 = lightProjectionMatrix * lightView1 * modelMatrix1;

                    m_light1.shadowMap.bindFramebuffer();
                    for(GPUMesh& mesh: m_meshes){
                        m_shadowShader.bind();
                        glUniformMatrix4fv(m_shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(lightMVP1));
                        mesh.draw(m_shadowShader);
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);

                    for (GPUMesh& mesh : m_meshes) {
                        m_lambertShader.bind();
                        glUniformMatrix4fv(m_lambertShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                        glUniformMatrix4fv(m_lambertShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                        glUniformMatrix3fv(m_lambertShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                        glUniformMatrix4fv(m_lambertShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                        glUniform3fv(m_lambertShader.getUniformLocation("lightPos"), 1, glm::value_ptr(m_light.position));
                        glUniform3fv(m_lambertShader.getUniformLocation("lightColor"), 1, glm::value_ptr(m_light.color));
                        glUniform1i(m_lambertShader.getUniformLocation("shadows"), m_shadow ? 1 : 0);
                        m_light.shadowMap.bindTexture(GL_TEXTURE7);
                        glUniform1i(m_lambertShader.getUniformLocation("shadowMap"), 7);
                        glUniform3fv(m_lambertShader.getUniformLocation("lightPos1"), 1, glm::value_ptr(m_light1.position));
                        glUniform3fv(m_lambertShader.getUniformLocation("lightColor1"), 1, glm::value_ptr(m_light1.color));
                        glUniformMatrix4fv(m_lambertShader.getUniformLocation("lightMVP1"), 1, GL_FALSE, glm::value_ptr(lightMVP1));
                        m_light1.shadowMap.bindTexture(GL_TEXTURE8);
                        glUniform1i(m_lambertShader.getUniformLocation("shadowMap1"), 8);
                        mesh.draw(m_lambertShader);
                    }
                    renderLightsIcons(m_light, mvpMatrix);
                    renderLightsIcons(m_light1, mvpMatrix);
                }

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        std::cout << "Key pressed: " << key << std::endl;
        if(key == GLFW_KEY_L){
            m_light.position = m_camera.cameraPos();
            m_light.direction = m_camera.forward();
                }
        switch (key) {
            case GLFW_KEY_UP:
                m_robot.translate(m_robot.facingDirection());
                m_camera_third_person.movePosition(m_robot.facingDirection());
                m_camera_birds_eye.movePosition(m_robot.facingDirection());
                break;
            case GLFW_KEY_DOWN:
                m_robot.translate(-m_robot.facingDirection());
                m_camera_third_person.movePosition(-m_robot.facingDirection());
                m_camera_birds_eye.movePosition(-m_robot.facingDirection());
                break;
            case GLFW_KEY_LEFT:
                m_robot.translate(glm::normalize(glm::cross(glm::vec3(0, 1, 0), m_robot.facingDirection())));
                m_camera_third_person.movePosition(glm::normalize(glm::cross(glm::vec3(0, 1, 0), m_robot.facingDirection())));
                m_camera_birds_eye.movePosition(glm::normalize(glm::cross(glm::vec3(0, 1, 0), m_robot.facingDirection())));
                break;
            case GLFW_KEY_RIGHT:
                m_robot.translate(glm::normalize(-glm::cross(glm::vec3(0, 1, 0), m_robot.facingDirection())));
                m_camera_third_person.movePosition(-glm::normalize(glm::cross(glm::vec3(0, 1, 0), m_robot.facingDirection())));
                m_camera_birds_eye.movePosition(-glm::normalize(glm::cross(glm::vec3(0, 1, 0), m_robot.facingDirection())));
                break; 
            case GLFW_KEY_F:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_window.isKeyPressed(GLFW_KEY_RIGHT_CONTROL)){
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateLeftForearm(m_robotRotationAngle);
                    }else{
                        m_robot.rotateLeftForearm(-m_robotRotationAngle);
                    }
                }else{
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateRightForearm(m_robotRotationAngle);
                    }else{
                        m_robot.rotateRightForearm(-m_robotRotationAngle);
                    }
                }
                break;
            case GLFW_KEY_H:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                    m_robot.rotateHead(m_robotRotationAngle);
                }else{
                    m_robot.rotateHead(-m_robotRotationAngle);
                }
                break;
            case GLFW_KEY_U:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_window.isKeyPressed(GLFW_KEY_RIGHT_CONTROL)){
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateLeftUpperArm(m_robotRotationAngle);
                    }else{
                        m_robot.rotateLeftUpperArm(-m_robotRotationAngle);
                    }
                }else{
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateRightUpperArm(m_robotRotationAngle);
                    }else{
                        m_robot.rotateRightUpperArm(-m_robotRotationAngle);
                    }
                }
                break;
            case GLFW_KEY_K:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_window.isKeyPressed(GLFW_KEY_RIGHT_CONTROL)){
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateLeftFoot(m_robotRotationAngle);
                    }else{
                        m_robot.rotateLeftFoot(-m_robotRotationAngle);
                    }
                }else{
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateRightFoot(m_robotRotationAngle);
                    }else{
                        m_robot.rotateRightFoot(-m_robotRotationAngle);
                    }
                }
                break;
            case GLFW_KEY_T:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_window.isKeyPressed(GLFW_KEY_RIGHT_CONTROL)){
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateLeftThigh(m_robotRotationAngle);
                    }else{
                        m_robot.rotateLeftThigh(-m_robotRotationAngle);
                    }
                }else{
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateRightThigh(m_robotRotationAngle);
                    }else{
                        m_robot.rotateRightThigh(-m_robotRotationAngle);
                    }
                }
                break;
            case GLFW_KEY_J:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_window.isKeyPressed(GLFW_KEY_RIGHT_CONTROL)){
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateLeftHand(m_robotRotationAngle);
                    }else{
                        m_robot.rotateLeftHand(-m_robotRotationAngle);
                    }
                }else{
                    if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                        m_robot.rotateRightHand(m_robotRotationAngle);
                    }else{
                        m_robot.rotateRightHand(-m_robotRotationAngle);
                    }
                }
                break;
            case GLFW_KEY_R:
                if(m_window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)){
                    m_robot.rotate(m_robotRotationAngle, glm::vec3(0.0, 1.0, 0.0));
                    m_camera_third_person.rotateAround(m_robot.centerVectors[1], m_robotRotationAngle, glm::vec3(0.0,1.0,0.0));
                }else{
                    m_robot.rotate(-m_robotRotationAngle, glm::vec3(0.0, 1.0, 0.0));
                    m_camera_third_person.rotateAround(m_robot.centerVectors[1], -m_robotRotationAngle, glm::vec3(0.0,1.0,0.0));
                }
                break;
            default:
                std::cout << "Key pressed: " << key << std::endl;
        }
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {
        switch (key) {
            default:
                std::cout << "Key released: " << key << std::endl;
        }
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
        //std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
        std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        std::cout << "Released mouse button: " << button << std::endl;
    }

    void checkWireframe() {
        if(m_wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

private:
    Window m_window;

    // Light(s)
    Light m_light;
    Light m_light1;

    // Cameras
    Camera m_camera;
    Camera m_camera_birds_eye;
    Camera m_camera_third_person;
    ViewMode currentViewMode = DEFAULT;
    Minimap m_minimap;

    bool isDragging = false;
    double lastX = 0.0f, lastY = 0.0f;

    ParticleSystem fire;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_pbrShader;
    Shader m_bezierShader;
    Shader m_pointOnBezierShader;

    std::vector<GPUMesh> m_meshes;
    Curve m_bezier;
    Curve m_bezier1;
    Curve m_bezier2;
    GPUCurve m_curve;
    GPUCurve m_curve1;
    GPUCurve m_curve2;
    Shader m_environmentMappingShader;
    Shader m_reflectionShader;
    Shader m_refractionShader;
    Shader m_lambertShader;

    // wireframing
    bool m_wireframe = false;

    // Tarrain data
    Terrain m_terrain;

    std::vector<GPUMesh> m_meshes_cube;
    Texture m_texture;
    Robot m_robot;
    glm::vec3 m_robotTranslateVector;
    glm::vec3 m_robotRotationAxis;
    float m_robotRotationAngle;

    CubeMapTexture m_cubemap;

    PBRTextures m_pbrTextures;
    std::filesystem::path m_pbrTexturesDirectory;
    bool m_usePBRShading {false};
    bool m_useDefaultShading {false};
    bool m_useMaterial { true };
    bool m_constantSpeedOnPath {false};
    std::map<float, float> m_bezierLookup;

    bool m_useEnvironmentMapping { true };
    bool m_useReflectionShading {false};
    bool m_useRefractionShading {true};
    bool m_useNormalMapping {true};
    bool m_shadow { true };
    bool m_lambert { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 200.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix { 1.0f };
};

int main()
{
    Application app;
    app.update();

    return 0;
}
