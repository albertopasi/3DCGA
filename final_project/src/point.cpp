#include "point.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>

GPUPoint::GPUPoint(const glm::vec3& cpuPoint)
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cpuPoint), &cpuPoint, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

}

GPUPoint::GPUPoint(GPUPoint&& other)
{
    moveInto(std::move(other));
}

GPUPoint::~GPUPoint()
{
    freeGpuMemory();
}

GPUPoint& GPUPoint::operator=(GPUPoint&& other)
{
    moveInto(std::move(other));
    return *this;
}

void GPUPoint::draw(const Shader& drawingShader)
{
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, 1);
}

//Move VAO and VBO
void GPUPoint::moveInto(GPUPoint&& other)
{
    freeGpuMemory();
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;

    other.m_vbo = INVALID;
    other.m_vao = INVALID;
}

//Free GPU
void GPUPoint::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
}