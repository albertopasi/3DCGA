#pragma once

// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
#include <framework/mesh.h>
#include <framework/shader.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
DISABLE_WARNINGS_POP()

#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>

struct Curve {
	std::vector<glm::vec3> controlPoints;
	glm::vec4 color;
};

class GPUCurve {
public:
    GPUCurve(const Curve& cpuCurve);
    // Cannot copy a GPU  curve because it would require reference counting of GPU resources.
    GPUCurve(const GPUCurve&) = delete;
    GPUCurve(GPUCurve&&);
    ~GPUCurve();

    // Cannot copy a GPU  curve because it would require reference counting of GPU resources.
    GPUCurve& operator=(const GPUCurve&) = delete;
    GPUCurve& operator=(GPUCurve&&);

    // Bind VAO and call glDrawElements.
    void draw(const Shader& drawingShader);

private:
    void moveInto(GPUCurve&&);
    void freeGpuMemory();

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;

    GLuint m_vbo { INVALID };
    GLuint m_vao { INVALID };
};
