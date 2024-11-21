#ifndef PARTICLE_SYSTEM_H
#define	PARTICLE_SYSTEM_H

#include "texture.h"
#include <framework/shader.h>
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

class ParticleSystem
{
public:
    ParticleSystem();
    
    ~ParticleSystem();
    
    void InitParticleSystem(const glm::vec3& Pos);
    
    void Render(int DeltaTimeMillis, const glm::mat4& VP, const glm::vec3& CameraPos);
    
private:
    //Update particles based on time
    void UpdateParticles(int DeltaTimeMillis);
    //Render particles form camera position
    void RenderParticles(const glm::mat4& VP, const glm::vec3& CameraPos);
    
    bool m_isFirst;
    unsigned int m_currVB;
    unsigned int m_currTFB;
    GLuint m_particleBuffer[2];
    GLuint m_transformFeedback[2];
    Shader m_updateShader;
    Shader m_billboardShader;
    RandomTexture m_randomTexture;
    Texture m_pTexture;
    int m_time;
};

#endif