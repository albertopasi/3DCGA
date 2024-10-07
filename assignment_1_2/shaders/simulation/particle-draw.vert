#version 410

uniform sampler2D positions;
uniform sampler2D velocities;
uniform sampler2D bounceData;
uniform mat4 viewProjection;
uniform uint numParticles;
uniform float particleRadius;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragVelocity;
layout(location = 3) out vec3 fragBounceData;

void main() {
    // Fetch position and velocity of particle from position texture
    // Half texel step ensures that we sample at the center of the relevant texel
    float halfTexelStep     = 1.0f / (numParticles * 2.0f);
    vec2 dataTexIdx         = vec2((float(gl_InstanceID) + halfTexelStep) / float(numParticles), 0);
    vec3 particlePosition   = texture(positions, dataTexIdx).xyz;
    vec3 particleVelocity   = texture(velocities, dataTexIdx).xyz;
    vec3 particleBounceData = texture(bounceData, dataTexIdx).rgb;
    
    // Compute world-space and NDC coordinates
    vec3 worldSpacePosition = (position * particleRadius) + particlePosition;
    gl_Position             = viewProjection * vec4(worldSpacePosition, 1);
    
    // Set output variables
    fragPosition    = worldSpacePosition;
    fragNormal      = normal;
    fragVelocity    = particleVelocity;
    fragBounceData  = particleBounceData;
}
