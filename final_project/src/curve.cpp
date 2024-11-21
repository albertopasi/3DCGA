#include "curve.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>

GPUCurve::GPUCurve(const Curve& cpuCurve)
{   //Generate and bind VAO and VBO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Upload the control points from CPU to GPU memory
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuCurve.controlPoints.size() * sizeof(decltype(cpuCurve.controlPoints)::value_type)), cpuCurve.controlPoints.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    // Set the number of vertices per patch for tessellation
    glPatchParameteri(GL_PATCH_VERTICES, 4);
}

GPUCurve::GPUCurve(GPUCurve&& other)
{
    moveInto(std::move(other));
}

GPUCurve::~GPUCurve()
{
    freeGpuMemory();
}

GPUCurve& GPUCurve::operator=(GPUCurve&& other)
{
    moveInto(std::move(other));
    return *this;
}

void GPUCurve::draw(const Shader& drawingShader)
{
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    //drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);
    
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawArrays(GL_PATCHES, 0, 4);
}

void GPUCurve::moveInto(GPUCurve&& other)
{
    freeGpuMemory();
    //m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    //m_uboMaterial = other.m_uboMaterial;

    //other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    //other.m_uboMaterial = INVALID;
}

void GPUCurve::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
}