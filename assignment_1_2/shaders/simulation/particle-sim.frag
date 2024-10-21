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
uniform int bounceThreshold;
uniform int bounceFrames;

layout(location = 0) out vec3 finalPosition;
layout(location = 1) out vec3 finalVelocity;
layout(location = 2) out vec3 finalBounceData;

void main() {
    // Fetch particle's current position and velocity
    vec3 currentPosition = texture(previousPositions, gl_FragCoord.xy/textureSize(previousPositions,0)).xyz;
    vec3 currentVelocity = texture(previousVelocities, gl_FragCoord.xy/textureSize(previousVelocities,0)).xyz;
    vec3 currentBounceData = texture(previousBounceData, gl_FragCoord.xy/textureSize(previousBounceData ,0)).xyz;

    // ===== Task 1.1 Verlet Integration =====
    vec3 acceleration = vec3(0.0, -9.81, 0.0);

    finalPosition = currentPosition + (currentVelocity * timestep) + (0.5 * acceleration * timestep * timestep).xyz;
    finalVelocity = currentVelocity + (acceleration * timestep);

    float offset = 0.001;

    // blinking matters
    if(currentBounceData.x >= bounceThreshold){
        currentBounceData.x = 0;
        currentBounceData.y = bounceFrames;
    }
    
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
                vec3 collisionPoint = vec3((finalPosition+othersPosition)/2);
                normalCollision = normalize(finalPosition-collisionPoint);
                finalPosition += normalCollision * offset;
                finalVelocity = reflect(currentVelocity, normalCollision);
                
                currentBounceData.x ++;
            }
        }
    }
    
    // ===== Task 1.2 Container Collision =====
    
    float distanceCenters = distance(containerCenter, finalPosition);
    float diffRadius = containerRadius - particleRadius;

    if(distanceCenters >= diffRadius){
        vec3 normal = normalize(containerCenter - finalPosition);
        float outsideDepth = distanceCenters - diffRadius;
        finalPosition += normal * outsideDepth;
        normal = normalize(containerCenter - finalPosition);
        vec3 collisionPoint = finalPosition - normal*particleRadius;
        normal = normalize(containerCenter - collisionPoint);
        finalPosition += normal * offset;
        finalVelocity = reflect (finalVelocity, normal);

        currentBounceData.x ++;
    }

    if(currentBounceData.y > 0){
        currentBounceData.y --;
    }

    finalBounceData = vec3(currentBounceData.x, currentBounceData.y, 0.0);
}
