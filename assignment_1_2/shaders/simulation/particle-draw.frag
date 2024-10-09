#version 410

uniform vec3 containerCenter;
uniform vec3 partMinSpeedColor;
uniform vec3 partMaxSpeedColor;
uniform float colorMaxSpeed;
uniform bool useSpeedBasedColor;
uniform bool useShading;
uniform float ambientCoef;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragVelocity;
layout(location = 3) in vec3 fragBounceData;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 baseColor = vec3(1.0);

    // ===== Task 2.1 Speed-based Colors =====

    if(useSpeedBasedColor) {
        float speed = length(fragVelocity);
        float t = clamp(speed / colorMaxSpeed, 0.0f, 1.0f);
        vec3 color = mix(partMinSpeedColor, partMaxSpeedColor, t);

        baseColor = color;
    }



    vec3 finalColor = baseColor;

    // ===== Task 2.2 Shading =====

    if (useShading) {
        vec3 lightDir = normalize(containerCenter - fragPosition);

        float diffuseColor = max(dot(fragNormal, lightDir), 0.0);

        vec3 ambient = baseColor * ambientCoef;
        vec3 diffuse = diffuseColor * baseColor;
        baseColor = ambient + diffuse;
    }

    finalColor = baseColor;

    fragColor = vec4(finalColor, 1.0);
}
