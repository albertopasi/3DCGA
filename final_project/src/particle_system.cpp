#include "particle_system.h"

#define ZERO_MEM(a) memset(a, 0, sizeof(a))

#define MAX_PARTICLES 5000
#define PARTICLE_LIFETIME 10.0f

#define PARTICLE_TYPE_LAUNCHER 0.0f
#define PARTICLE_TYPE_SHELL 1.0f
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f

struct Particle
{
    float Type;    
    glm::vec3 Pos;
    glm::vec3 Vel;    
    float LifetimeMillis;    
};


ParticleSystem::ParticleSystem(): 
    m_currVB(0), 
    m_currTFB(1),
    m_isFirst(true),
    m_time(0),
    m_pTexture(RESOURCE_ROOT "resources/fireAtlas.png"),
    m_randomTexture()
{            
    ZERO_MEM(m_transformFeedback);
    ZERO_MEM(m_particleBuffer);
}

//Destructor of particle system
ParticleSystem::~ParticleSystem()
{   
    if (m_transformFeedback[0] != 0) {
        glDeleteTransformFeedbacks(2, m_transformFeedback);
    }
    
    if (m_particleBuffer[0] != 0) {
        glDeleteBuffers(2, m_particleBuffer);
    }
}

//Init, bind buffer and add shaders
void ParticleSystem::InitParticleSystem(const glm::vec3& Pos)
{   
    Particle Particles[MAX_PARTICLES];
    ZERO_MEM(Particles);

    Particles[0].Type = PARTICLE_TYPE_LAUNCHER;
    Particles[0].Pos = Pos;
    Particles[0].Vel = glm::vec3(0.0f, -0.0001f, 0.0f);
    Particles[0].LifetimeMillis = 0.0f;

    glGenTransformFeedbacks(2, m_transformFeedback);    
    glGenBuffers(2, m_particleBuffer);
    
    for (unsigned int i = 0; i < 2 ; i++) {
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback[i]);
        glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Particles), Particles, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_particleBuffer[i]);        
    }
    
    ShaderBuilder updateBuilder;
    updateBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/update_vert.glsl");
    updateBuilder.addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/update_geom.glsl");
    updateBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/update_frag.glsl");
    
    const GLchar* varyings[4];    
    varyings[0] = "Type1";
    varyings[1] = "Position1";
    varyings[2] = "Velocity1";    
    varyings[3] = "Age1";

    m_updateShader = updateBuilder.transformFeedbackBuild(varyings, 4);
    // m_updateShader.transformFeedback(varyings, 4);                                        

    ShaderBuilder billboardBuilder;
    billboardBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/billboard_vert.glsl");
    billboardBuilder.addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/billboard_geom.glsl");
    billboardBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/billboard_frag.glsl");
    m_billboardShader = billboardBuilder.build();                                                     
    
}

//Calls update and render functions, after creating a VAO
void ParticleSystem::Render(int DeltaTimeMillis, const glm::mat4& VP, const glm::vec3& CameraPos)
{
    m_time += DeltaTimeMillis;

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    UpdateParticles(DeltaTimeMillis);

    RenderParticles(VP, CameraPos);

    m_currVB = m_currTFB;
    m_currTFB = (m_currTFB + 1) & 0x1;
}

//Update particles based on time
void ParticleSystem::UpdateParticles(int DeltaTimeMillis)
{
    // Bind the update shader to update particle properties
    m_updateShader.bind();
    glUniform1f(m_updateShader.getUniformLocation("gDeltaTimeMillis"), DeltaTimeMillis);
    glUniform1f(m_updateShader.getUniformLocation("gTime"), m_time);
    glUniform1f(m_updateShader.getUniformLocation("gLauncherLifetime"), 50.0f);
    glUniform1f(m_updateShader.getUniformLocation("gShellLifetime"), 5000.0f);

    //glUniform1f(m_updateShader.getUniformLocation("gSecondaryShellLifetime"), 2500.0f);    
    m_randomTexture.bind(GL_TEXTURE9);
    glUniform1i(m_updateShader.getUniformLocation("gRandomTexture"), 9);

    
    // Disable rasterizer to only use transform feedback without rendering geometry to screen
    glEnable(GL_RASTERIZER_DISCARD);

    glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[m_currVB]);    
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback[m_currTFB]);

    // Enable attributes for particle data: type, position, velocity, lifetime
    glEnableVertexAttribArray(0);                    
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // Set up vertex attributes to match the Particle structure
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), 0);                          // type
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)4);         // position
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)16);        // velocity
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)28);          // lifetime
    
    glBeginTransformFeedback(GL_POINTS);

    // Draw particles based on whether it's the first frame or not
    if (m_isFirst) {
        glDrawArrays(GL_POINTS, 0, 1);

        m_isFirst = false;
    }
    else {
        glDrawTransformFeedback(GL_POINTS, m_transformFeedback[m_currVB]);
    }            
    
    glEndTransformFeedback();

    // Disable attribute arrays after update
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
}
    

void ParticleSystem::RenderParticles(const glm::mat4& VP, const glm::vec3& cameraPos)
{
    // Bind the shader used for rendering billboards (particles as sprites)
    m_billboardShader.bind();
    glUniform1f(m_billboardShader.getUniformLocation("gBillboardSize"), 0.05f);
    glUniform3fv(m_billboardShader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));
    glUniformMatrix4fv(m_billboardShader.getUniformLocation("gVP"), 1, GL_FALSE, glm::value_ptr(VP));
    glUniform1f(m_billboardShader.getUniformLocation("maxLife"), 5000.0f);

    // Bind the particle texture
    m_pTexture.bind(GL_TEXTURE7);
    glUniform1i(m_billboardShader.getUniformLocation("gColorMap"), 7);
    // Re-enable rasterization for rendering the particles as billboards
    glDisable(GL_RASTERIZER_DISCARD);
    // Bind the particle buffer for rendering and enable relevant attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_particleBuffer[m_currTFB]);    

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Set up vertex attributes for rendering: position, lifetime
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)4);  // position
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)28); // lifetime


    glDrawTransformFeedback(GL_POINTS, m_transformFeedback[m_currTFB]);

    // Disable attribute arrays after rendering
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}