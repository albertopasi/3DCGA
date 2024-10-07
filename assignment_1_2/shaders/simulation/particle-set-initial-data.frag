#version 410

#define M_PI 3.1415926535897932384626433832795
#define M_2PI 6.2831853071795864769252867665590

uniform uint numParticles;
uniform float particleRadius;
uniform vec3 containerCenter;
uniform float containerRadius;

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout(location = 0) in vec2 bufferCoords;

layout(location = 0) out vec3 initialPosition;
layout(location = 1) out vec3 initialVelocity;
layout(location = 2) out vec3 initialBounceData;


float rand(vec2 n) { return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453); }

void main() {
    // Evenly-ish distribute on unit sphere
    float particleIdxFrac   = (gl_FragCoord.x / numParticles);
    float inclination       = particleIdxFrac * M_PI;
    float azimuth           = particleIdxFrac * M_2PI;
    vec3 randomDirection    = vec3(sin(inclination) * cos(azimuth),
                                   sin(inclination) * sin(azimuth),
                                   cos(inclination));

    // Figure out particle location and place it there
    float randomFactor          = rand(vec2(42, gl_FragCoord.x));
    vec3 containerCenterOffset  = (randomFactor * (containerRadius - particleRadius)) // Prevents container-particle intersection
                                  * randomDirection;
    initialPosition             = containerCenter + containerCenterOffset;

    // Zero out velocity
    initialVelocity = vec3(0.0f);

    // Zero out bounce data
    initialBounceData = vec3(0.0f);
}
