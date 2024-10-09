#version 410
#extension GL_ARB_explicit_uniform_location : enable

uniform sampler2D previousPositions;
uniform sampler2D previousVelocities;
uniform sampler2D previousBounceData;
uniform float timestep;
uniform uint numParticles;
uniform float particleRadius;
uniform vec3 containerCenter;
uniform float containerRadius;
uniform bool interParticleCollision;

layout(location = 0) out vec3 finalPosition;
layout(location = 1) out vec3 finalVelocity;
layout(location = 2) out vec3 finalBounceData;

void main() {

    // Fetch particle's current position and velocity
    ivec2 fragCoord = ivec2(gl_FragCoord.xy);
    vec3 currentPosition = texelFetch(previousPositions, fragCoord, 0).xyz;
    vec3 currentVelocity = texelFetch(previousVelocities, fragCoord, 0).xyz;

    // ===== Task 1.1 Verlet Integration =====
    vec3 acceleration = vec3(0.0, -9.81, 0.0);

    finalPosition = currentPosition + (currentVelocity * timestep) + (0.5 * acceleration * timestep * timestep);
    finalVelocity = currentVelocity + (acceleration * timestep);


    // ===== Task 1.3 Inter-particle Collision =====
    if (interParticleCollision) {
        for(int i=0; i<numParticles; i++){

            float index = float(i) / float(numParticles);
            vec3 othersPosition = texture(previousPositions, vec2(index, 0.5)).xyz;

            if(currentPosition == othersPosition) continue;

            vec3 normalCollision = normalize(finalPosition - othersPosition);
            float distanceOthers = distance(finalPosition,othersPosition);

            if(distanceOthers < 2.0*particleRadius){
                float penetrationDepth = 2*particleRadius - distanceOthers;
                finalPosition += normalCollision * penetrationDepth;
                finalVelocity = reflect(currentVelocity, normalCollision);
            }
        }
    }

    
    // ===== Task 1.2 Container Collision =====
    
    float offset = 0.001;
    float distanceCenters = distance(containerCenter, finalPosition);
    float diffRadius = containerRadius - particleRadius;

    if(distanceCenters >= diffRadius){
        vec3 normal = normalize(containerCenter - finalPosition);
        float outsideDepth = distanceCenters - diffRadius;
        finalPosition += normal * (outsideDepth + offset);
        finalVelocity = reflect (finalVelocity, normal);
    }






}
