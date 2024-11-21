#version 410 core

in vec2 fragTexCoord;
in float timeOffset; // Time offset for unique animation per particle

out vec4 FragColor;

uniform sampler2D fireTexture; // Fire color texture
uniform sampler2D alphaTexture;
uniform sampler2D noiseTexture; // Noise texture for randomness
uniform float time;

void main()
{
    // Sample noise texture and animate over time
    vec2 animatedTexCoords = fragTexCoord + vec2(time * 0.1 + timeOffset, time * 0.1 + timeOffset);
    float noise = texture(noiseTexture, animatedTexCoords).r;

    // Sample the base fire color texture
    vec4 fireColor = texture(fireTexture, fragTexCoord);

    // Modulate fire color and alpha by noise to add randomness
    fireColor.rgb *= (0.7 + 0.3 * noise);  // Adjust brightness randomly
    fireColor.a *= noise;                  // Adjust alpha with noise for organic transparency

    // Use an alpha gradient for smooth edges
    float edgeAlpha = texture(alphaTexture, fragTexCoord).r;
    //fireColor.a *= edgeAlpha;

    // Animate color over life: start bright yellow-orange, end in dark red
    float lifeFactor = timeOffset;                   // Simulates life stage (0 = start, 1 = end)
    vec3 startColor = vec3(1.0, 0.8, 0.5);           // Start as bright yellow-orange
    vec3 endColor = vec3(0.7, 0.1, 0.0);             // End as dark red
    fireColor.rgb = mix(endColor, startColor, lifeFactor); // Interpolate color

    // Output final fragment color
    FragColor = fireColor;
}
