// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// #define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <cstdlib> // EXIT_FAILURE
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <toml/toml.hpp>
#include <vector>
#include <array>

// Configuration
const int WIDTH = 1200;
const int HEIGHT = 800;

bool show_imgui = true;
bool debug = true;
bool diffuseLighting = false;
bool phongSpecularLighting = false;
bool blinnPhongSpecularLighting = false;
bool toonLightingDiffuse = false;
bool toonLightingSpecular = false;
bool toonxLighting = false;

bool do_pcf = false;
bool do_shadows = false;
int samplingMode = 0;
int lightMode = 0;
int lightColorMode = 0;

enum class DiffuseModel {
    debug=0,
    lambert=1,
    toon=2,
    xtoon=3
};
enum class SpecularModel {
    none=0,
    phong=1,
    blinnphong=2,
    toon=3
};
DiffuseModel selectedDiffuseModel { DiffuseModel::debug };
SpecularModel selectedSpecularModel { SpecularModel::none };

struct {
    // Diffuse (Lambert)
    glm::vec3 kd { 0.5f };
    // Specular (Phong/Blinn Phong)
    glm::vec3 ks { 0.5f };
    float shininess = 3.0f;
    // Toon
    int toonDiscretize = 4;
    float toonSpecularThreshold = 0.49f;
} shadingData;

struct Texture {
    int width;
    int height;
    int channels;
    stbi_uc* texture_data;
};

// Lights
struct Light {
    glm::vec3 position;
    glm::vec3 color;
    bool is_spotlight;
    glm::vec3 direction;
    bool has_texture;
    Texture texture;
};

std::vector<Light> lights {};
size_t selectedLightIndex = 0;
std::vector<Light> defaultLights = {};

void resetLights()
{
    lights.clear();
    lights = defaultLights;
    selectedLightIndex = 0;
}

// Add a new light
void addLight() {
    lights.push_back(Light{glm::vec3(0.0, 0.0, 3.0), glm::vec3(1.0), false, glm::vec3(0,0,1), false }); // Add a default light
    selectedLightIndex = lights.size() - 1; // Select the newly added light
}

// Remove the selected light
void removeLight() {
    if (!lights.empty() && lights.size() > 1) {
        lights.erase(lights.begin() + selectedLightIndex);
        selectedLightIndex = std::min(selectedLightIndex, lights.size() - 1); // Ensure valid index
    }
}

void imgui()
{

    // Define UI here
    if (!show_imgui)
        return;

    ImGui::Begin("Final project part 1 : Modern Shading");
    ImGui::Text("Press \\ to show/hide this menu");

    ImGui::Separator();
    ImGui::Text("Material parameters");

    // Color pickers for Kd and Ks
    ImGui::ColorEdit3("Kd", &shadingData.kd[0]);
    ImGui::ColorEdit3("Ks", &shadingData.ks[0]);
    ImGui::SliderFloat("Shininess", &shadingData.shininess, 0.0f, 100.f);
    ImGui::SliderInt("Toon Discretization", &shadingData.toonDiscretize, 1, 10);
    ImGui::SliderFloat("Toon Specular Threshold", &shadingData.toonSpecularThreshold, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Lights");

    // Display lights in scene
    std::vector<std::string> itemStrings = {};
    for (size_t i = 0; i < lights.size(); i++) {
        auto string = "Light " + std::to_string(i);
        itemStrings.push_back(string);
    }

    std::vector<const char*> itemCStrings = {};
    for (const auto& string : itemStrings) {
        itemCStrings.push_back(string.c_str());
    }

    int tempSelectedItem = static_cast<int>(selectedLightIndex);
    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int) itemCStrings.size(), 4)) {
        selectedLightIndex = static_cast<size_t>(tempSelectedItem);
    }

    // Add/Remove/Reset lights
    if (ImGui::Button("Add Light")) {
        addLight();
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Light")) {
        removeLight();
    }
    if (ImGui::Button("Reset Lights")) {
        resetLights();
    }

    if (!lights.empty()) {
        Light& selectedLight = lights[selectedLightIndex];

        // Position of the light
        ImGui::DragFloat3("Position", &selectedLight.position[0], 0.1f);

        // Color of the light
        ImGui::ColorEdit3("Color", &selectedLight.color[0]);
    }
    
    ImGui::Separator();
    ImGui::Text("Render Settings");

    std::array diffuseModels { "debug", "lambert", "toon", "x-toon" };
    int current_diffuse = static_cast<int>(selectedDiffuseModel);
    ImGui::Combo("Diffuse Model", &current_diffuse, diffuseModels.data(), diffuseModels.size());
    selectedDiffuseModel = static_cast<DiffuseModel>(current_diffuse);

    // Update lighting modes based on selected diffuse model
    switch (selectedDiffuseModel) {
        case DiffuseModel::debug:
            debug = true;
            diffuseLighting = false;
            toonLightingDiffuse = false;
            toonxLighting = false;
            break;
        case DiffuseModel::lambert:
            debug = false;
            diffuseLighting = true;
            toonLightingDiffuse = false;
            toonxLighting = false;
            break;
        case DiffuseModel::toon:
            debug = false;
            diffuseLighting = false;
            toonLightingDiffuse = true;
            toonxLighting = false;
            break;
        case DiffuseModel::xtoon:
            debug = false;
            diffuseLighting = false;
            toonLightingDiffuse = false;
            toonxLighting = true;
            break;
    }

    std::array specularModels { "none", "phong", "blinn-phong", "toon" };
    int current_specular = static_cast<int>(selectedSpecularModel);
    ImGui::Combo("Specular Model", &current_specular, specularModels.data(), specularModels.size());
    selectedSpecularModel = static_cast<SpecularModel>(current_specular);

    // Update specular lighting modes based on selected specular model
    switch (selectedSpecularModel) {
        case SpecularModel::none:
            phongSpecularLighting = false;
            blinnPhongSpecularLighting = false;
            toonLightingSpecular = false;
            break;
        case SpecularModel::phong:
            phongSpecularLighting = true;
            blinnPhongSpecularLighting = false;
            toonLightingSpecular = false;
            break;
        case SpecularModel::blinnphong:
            phongSpecularLighting = false;
            blinnPhongSpecularLighting = true;
            toonLightingSpecular = false;
            break;
        case SpecularModel::toon:
            phongSpecularLighting = false;
            blinnPhongSpecularLighting = false;
            toonLightingSpecular = true;
            break;
    }

    ImGui::Separator();
    ImGui::Text("Shadows");
    ImGui::Checkbox("Shadows", &do_shadows);
    ImGui::SameLine();
    ImGui::BeginDisabled(!do_shadows);
    ImGui::Checkbox("PCF", &do_pcf);
    ImGui::EndDisabled();

    ImGui::End();
    ImGui::Render();
}

std::optional<glm::vec3> tomlArrayToVec3(const toml::array* array)
{
    glm::vec3 output {};

    if (array) {
        int i = 0;
        array->for_each([&](auto&& elem) {
            if (elem.is_number()) {
                if (i > 2)
                    return;
                output[i] = static_cast<float>(elem.as_floating_point()->get());
                i += 1;
            } else {
                std::cerr << "Error: Expected a number in array, got " << elem.type() << std::endl;
                return;
            }
        });
    }
    return output;
}

// Program entry point. Everything starts here.
int main(int argc, char** argv)
{
    // read toml file from argument line (otherwise use default file)
    std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/default_scene.toml";

    // parse initial scene config
    toml::table config;
    try {
        config = toml::parse_file(std::string(RESOURCE_ROOT) + config_filename);
    } catch (const toml::parse_error& ) {
        std::cerr << "parsing failed" << std::endl;
    }

    // read material data
    shadingData.kd = tomlArrayToVec3(config["material"]["kd"].as_array()).value();
    shadingData.ks = tomlArrayToVec3(config["material"]["ks"].as_array()).value();
    shadingData.shininess = config["material"]["shininess"].value_or(0.0f);
    shadingData.toonDiscretize = (int) config["material"]["toonDiscretize"].value_or(0);
    shadingData.toonSpecularThreshold = config["material"]["toonSpecularThreshold"].value_or(0.0f);

    // read lights
    lights = std::vector<Light> {};
    size_t num_lights = config["lights"]["positions"].as_array()->size();
    std::cout << num_lights << std::endl;

    for (size_t i = 0; i < num_lights; ++i) {
        auto pos = tomlArrayToVec3(config["lights"]["positions"][i].as_array()).value();
        auto color = tomlArrayToVec3(config["lights"]["colors"][i].as_array()).value();
        bool is_spotlight = config["lights"]["is_spotlight"][i].value<bool>().value();
        auto direction = tomlArrayToVec3(config["lights"]["direction"][i].as_array()).value();
        bool has_texture = config["lights"]["has_texture"][i].value<bool>().value();

        auto tex_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
        int width = 0, height = 0, sourceNumChannels = 0;// Number of channels in source image. pixels will always be the requested number of channels (3).
        stbi_uc* pixels = nullptr;        
        if (has_texture) {
            pixels = stbi_load(tex_path.c_str(), &width, &height, &sourceNumChannels, STBI_rgb);
        }

        defaultLights.emplace_back(Light { pos, color, is_spotlight, direction, has_texture, { width, height, sourceNumChannels, pixels } });
    }
    std::copy(defaultLights.begin(), defaultLights.end(), std::back_inserter(lights));

    // Create window
    Window window { "Shading", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };

    // read camera settings
    auto look_at = tomlArrayToVec3(config["camera"]["lookAt"].as_array()).value();
    auto rotations = tomlArrayToVec3(config["camera"]["rotations"].as_array()).value();
    float fovY = config["camera"]["fovy"].value_or(50.0f);
    float dist = config["camera"]["dist"].value_or(1.0f);

    auto diffuse_model = config["render_settings"]["diffuse_model"].value<std::string>();
    auto specular_model = config["render_settings"]["specular_model"].value<std::string>();
    do_pcf = config["render_settings"]["pcf"].value<bool>().value();
    do_shadows = config["render_settings"]["shadows"].value<bool>().value();

    std::cout << diffuse_model.value() << std::endl;

    if (diffuse_model.value() == "debug") {
        selectedDiffuseModel = DiffuseModel::debug;
        debug = true;
        diffuseLighting = false;
        toonLightingDiffuse = false;
        toonxLighting = false;
    } else if (diffuse_model.value() == "lambert") {
        selectedDiffuseModel = DiffuseModel::lambert;
        debug = false;
        diffuseLighting = true;
        toonLightingDiffuse = false;
        toonxLighting = false;
    } else if (diffuse_model.value() == "toon") {
        selectedDiffuseModel = DiffuseModel::toon;
        debug = false;
        diffuseLighting = false;
        toonLightingDiffuse = true;
        toonxLighting = false;
    } else if (diffuse_model.value() == "x-toon") {
        selectedDiffuseModel = DiffuseModel::xtoon;
        debug = false;
        diffuseLighting = false;
        toonLightingDiffuse = false;
        toonxLighting = true;
    }

    if (specular_model.value() == "none") {
        phongSpecularLighting = false;
        blinnPhongSpecularLighting = false;
        toonLightingSpecular = false;
    } else if (specular_model.value() == "phong") {
        phongSpecularLighting = true;
        blinnPhongSpecularLighting = false;
        toonLightingSpecular = false;
    } else if (specular_model.value() == "blinn-phong") {
        phongSpecularLighting = false;
        blinnPhongSpecularLighting = true;
        toonLightingSpecular = false;   
    } else if (specular_model.value() == "toon") {
        phongSpecularLighting = false;
        blinnPhongSpecularLighting = false;
        toonLightingSpecular = true;   
    }

    Trackball trackball { &window, glm::radians(fovY) };
    trackball.setCamera(look_at, rotations, dist);

    // read mesh
    bool animated = config["mesh"]["animated"].value_or(false);
    auto mesh_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");

    std::cout << mesh_path << std::endl;

    //const Mesh mesh = loadMesh(mesh_path)[0];
    const Mesh mesh = mergeMeshes(loadMesh(RESOURCE_ROOT "resources/scene.obj"));

    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (key == '\\' && action == GLFW_PRESS) {
            show_imgui = !show_imgui;
        }

        if (action != GLFW_RELEASE)
            return;
    });

    const Shader debugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl").build();
    const Shader lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl").build();
    const Shader lambertShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lambert_frag.glsl").build();
    const Shader phongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/phong_frag.glsl").build();
    const Shader blinnPhongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_phong_frag.glsl").build();
    const Shader toonDiffuseShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_diffuse_frag.glsl").build();
    const Shader toonSpecularShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_specular_frag.glsl").build();
    const Shader xToonShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/xtoon_frag.glsl").build();
    const Shader mainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").build();
    const Shader shadowShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").build();

    // Create Vertex Buffer Object and Index Buffer Objects.
    GLuint vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ibo;
    // Create index buffer object (IBO)
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Bind vertex data to shader inputs using their index (location).
    // These bindings are stored in the Vertex Array Object.
    GLuint vao;
    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
    // The stride is the distance in bytes between vertices. We use the offset to point to the normals
    // instead of the positions.
    // Tell OpenGL that we will be using vertex attributes 0 and 1.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // This is where we would set the attribute pointers, if apple supported it.

    glBindVertexArray(0);

    GLuint texShadow;
    const int SHADOWTEX_WIDTH = 1024;
    const int SHADOWTEX_HEIGHT = 1024;
    glGenTextures(1, &texShadow);
    glBindTexture(GL_TEXTURE_2D, texShadow);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Set behaviour for when texture coordinates are outside the [0, 1] range.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    // === Create framebuffer for extra texture ===
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texShadow, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    // Load image from disk to CPU memory.
    int width, height, sourceNumChannels; // Number of channels in source image. pixels will always be the requested number of channels (3).
    stbi_uc* pixels = stbi_load(RESOURCE_ROOT "resources/toon_map.png", &width, &height, &sourceNumChannels, STBI_rgb);


    // Create a texture on the GPU with 3 channels with 8 bits each.
    
    GLuint texToon;
    glGenTextures(1, &texToon);
    glBindTexture(GL_TEXTURE_2D, texToon);

    // Set behavior for when texture coordinates are outside the [0, 1] range.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Free the CPU memory after we copied the image to the GPU.
    stbi_image_free(pixels);

    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);

   // Main loop.
    while (!window.shouldClose()) {
        window.updateInput();

        imgui();

        // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set model/view/projection matrix.
        const glm::vec3 cameraPos = trackball.position();
        const glm::mat4 model { 1.0f };

        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();
        const glm::mat4 mvp = projection * view * model;



        const glm::mat4 lightView = glm::lookAt(lights[0].position, glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 lightMVP;
        //if(lights[0].is_spotlight){
            constexpr float fov = glm::pi<float>() / 4.0f;
            const float aspectRatio = static_cast<float>(window.getWindowSize().x) / static_cast<float>(window.getWindowSize().y);
            const glm::mat4 perspLightProjectionMatrix = glm::perspective(fov, aspectRatio, 0.01f, 30.0f);
            lightMVP = projection * lightView;
        // }else{
            // const glm::mat4 orthoLightProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
            // lightMVP = orthoLightProjectionMatrix * lightView;
        // }








        bool renderedSomething = false;
        auto render = [&](const Shader &shader) {
            renderedSomething = true;

            // Set the model/view/projection matrix that is used to transform the vertices in the vertex shader.
            glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

            // Bind vertex data.
            glBindVertexArray(vao);

            // We tell OpenGL what each vertex looks like and how they are mapped to the shader using the names
            // NOTE: Usually this can be stored in the VAO, since the locations would be the same in all shaders by using the layout(location = ...) qualifier in the shaders, however this does not work on apple devices.
            glVertexAttribPointer(shader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glVertexAttribPointer(shader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            // Execute draw command.
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

            glBindVertexArray(0);
        };


        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        shadowShader.bind();
        glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);

        glUniformMatrix4fv(shadowShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(lightMVP));

        glBindVertexArray(vao);

        glVertexAttribPointer(shadowShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        // Execute draw command
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size() * 3), GL_UNSIGNED_INT, nullptr);

        // Unbind the off-screen framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        mainShader.bind();
        glUniformMatrix4fv(mainShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(mainShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
        glUniform3fv(mainShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[0].position));
        //glUniform3fv(mainShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[0].color));

        glBindVertexArray(vao);
        glVertexAttribPointer(mainShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glVertexAttribPointer(mainShader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texShadow);
        glUniform1i(mainShader.getUniformLocation("texShadow"), 0);
        
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);

        glClearDepth(1.0);
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size() * 3), GL_UNSIGNED_INT, nullptr);

        if (!debug) {
            // Draw mesh into depth buffer but disable color writes.
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            debugShader.bind();
            render(debugShader);

            // Draw the mesh again for each light / shading model.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color writes.
            glDepthMask(GL_FALSE); // Disable depth writes.
            glDepthFunc(GL_EQUAL); // Only draw a pixel if it's depth matches the value stored in the depth buffer.
            glEnable(GL_BLEND); // Enable blending.
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending.

            for (const Light& light : lights) {
                renderedSomething = false;
                if (!renderedSomething) {
                    if (toonxLighting) {
                        xToonShader.bind();

                        // === SET YOUR X-TOON UNIFORMS HERE ===
                        // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos and texToon.

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, texToon);
                            glUniform3fv(xToonShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));
                            glUniform3fv(xToonShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                            glUniform1f(xToonShader.getUniformLocation("shininess"), shadingData.shininess);

                        glUniform1i(xToonShader.getUniformLocation("texToon"), 0); // Change xxx to the uniform name that you want to use.
                        render(xToonShader);

                    } else {
                        if (toonLightingDiffuse) {
                            toonDiffuseShader.bind();

                            // === SET YOUR DIFFUSE TOON UNIFORMS HERE ===
                            // Values that you may want to pass to the shader are stored in light, shadingData.
                            // 1. Pass the light's position to the shader
                            glUniform3fv(toonDiffuseShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));

                            // 2. Pass the light's color to the shader
                            glUniform3fv(toonDiffuseShader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));

                            // 3. Pass the diffuse reflection coefficient (kd) to the shader
                            glUniform3fv(toonDiffuseShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));

                            glUniform1i(toonDiffuseShader.getUniformLocation("toonDiscretize"), shadingData.toonDiscretize);

                            render(toonDiffuseShader);
                        }
                        if (toonLightingSpecular) {
                            toonSpecularShader.bind();

                            // === SET YOUR SPECULAR TOON UNIFORMS HERE ===
                            // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.

                            // 1. Pass the light's position to the shader
                            glUniform3fv(toonSpecularShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));

                            // 3. Pass the camera position to the shader
                            glUniform3fv(toonSpecularShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

                            // 5. Pass the shininess factor to the shader
                            glUniform1f(toonSpecularShader.getUniformLocation("shininess"), shadingData.shininess);

                            glUniform1f(toonSpecularShader.getUniformLocation("toonSpecularThreshold"), shadingData.toonSpecularThreshold);

                            render(toonSpecularShader);
                        }
                    }
                }
                if (!renderedSomething) {
                    if (diffuseLighting) {
                        lambertShader.bind();  // Bind the Lambert shader

                        // === SET YOUR LAMBERT UNIFORMS HERE ===

                        // 1. Pass the light's position to the shader
                        glUniform3fv(lambertShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));

                        // 2. Pass the light's color to the shader
                        glUniform3fv(lambertShader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));

                        // 3. Pass the diffuse reflection coefficient (kd) to the shader
                        glUniform3fv(lambertShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));

                        // Call the render function after setting the uniforms
                        render(lambertShader);
                    }
                    if (phongSpecularLighting || blinnPhongSpecularLighting) {
                        const Shader &shader = phongSpecularLighting ? phongShader : blinnPhongShader;
                        shader.bind();

                            // === SET YOUR PHONG/BLINN PHONG UNIFORMS HERE ===
                            // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                            // 1. Pass the light's position to the shader
                            glUniform3fv(shader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));

                            // 2. Pass the light's color to the shader
                            glUniform3fv(shader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));

                            // 3. Pass the camera position to the shader
                            glUniform3fv(shader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

                            // 4. Pass the specular reflection coefficient (ks) to the shader
                            glUniform3fv(shader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));

                            // 5. Pass the shininess factor to the shader
                            glUniform1f(shader.getUniformLocation("shininess"), shadingData.shininess);

                        render(shader);
                    }
                }
            }

            // Restore default depth test settings and disable blending.
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
        if (!renderedSomething) {
            debugShader.bind();
            //glUniform3fv(debugShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos)); // viewPos.
            render(debugShader);
        }

        // Draw lights as (square) points.
        lightShader.bind();
        {
            const glm::vec4 screenPos = mvp * glm::vec4(lights[selectedLightIndex].position, 1.0f);
            const glm::vec3 color { 1, 1, 0 };

            glPointSize(40.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(color));
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);       
        }
        for (const Light& light : lights) {
            const glm::vec4 screenPos = mvp * glm::vec4(light.position, 1.0f);
            // const glm::vec3 color { 1, 0, 0 };

            glPointSize(10.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(light.color));
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);       
        }

        // Present result to the screen.
        window.swapBuffers();
    }

    // Be a nice citizen and clean up after yourself.
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);

    return 0;
}