// tessellation evaluation shader
#version 410 core

layout (quads, fractional_odd_spacing, ccw) in;

uniform sampler2D heightMap;  // the texture corresponding to our height map
uniform mat4 mvpMatrix;

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoord[];

// send to Fragment Shader for coloring
out float Height;
out vec2 texCoord;

int octaves = 8;
float gDispFactor = 20.0;
float freq = 0.01f;

float power = 3.0f;

//vec3 seed = vec3(1.0f);

// float Random2D(in vec2 st) {
// 	return fract(sin(dot(st.xy, vec2(12.9898, 78.233) + seed.xy)) * 43758.5453123);
// }

// float InterpolatedNoise(int ind, float x, float y) {
// 	int integer_X = int(floor(x));
// 	int integer_Y = int(floor(y));
// 	float fractional_X = fract(x);
// 	float fractional_Y = fract(y);
// 	vec2 randomInput = vec2(integer_X, integer_Y);
// 	float a = Random2D(randomInput);
// 	float b = Random2D(randomInput + vec2(1.0, 0.0));
// 	float c = Random2D(randomInput + vec2(0.0, 1.0));
// 	float d = Random2D(randomInput + vec2(1.0, 1.0));

// 	vec2 w = vec2(fractional_X, fractional_Y);
// 	w = w * w * w * (10.0 + w * (-15.0 + 6.0 * w));

// 	float k0 = a,
// 	k1 = b - a,
// 	k2 = c - a,
// 	k3 = d - c - b + a;

// 	return k0 + k1 * w.x + k2 * w.y + k3 * w.x * w.y;
// }

// float perlin(float x, float y){
// 	int numOctaves = octaves;
// 	float persistence = 0.5;
// 	float total = 0,
// 	frequency = 0.005 * freq,
// 	amplitude = gDispFactor;
// 	for (int i = 0; i < numOctaves; ++i) {
// 		frequency *= 2.;
// 		amplitude *= persistence;

// 		total += InterpolatedNoise( 0, x * frequency, y * frequency) * amplitude;
// 	}
// 	return pow(total, power);
// }

// float perlin(vec2 st){
// 	const mat2 matrix = mat2(0.8,-0.6,0.6,0.8);
// 	float persistence = 0.5;
// 	float total = 0.0,
// 	frequency = 0.005*freq,
// 	amplitude = gDispFactor;
// 	for (int i = 0; i < octaves; ++i) {
// 		frequency *= 2.0;
// 		amplitude *= persistence;
// 		vec2 v = frequency * matrix * st;
// 		total += InterpolatedNoise(0, v.x, v.y) * amplitude;
// 	}
// 	return pow(total, power);
// }

/* Function to linearly interpolate between a0 and a1
 * Weight w should be in the range [0.0, 1.0]
 */
// float interpolate(float a0, float a1, float w) {
//     /* // You may want clamping by inserting:
//      * if (0.0 > w) return a0;
//      * if (1.0 < w) return a1;
//      */
//     return (a1 - a0) * w + a0;
//     /* // Use this cubic interpolation [[Smoothstep]] instead, for a smooth appearance:
//      * return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
//      *
//      * // Use [[Smootherstep]] for an even smoother result with a second derivative equal to zero on boundaries:
//      * return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
//      */
// }

// /* Create pseudorandom direction vector
//  */
// vec2 randomGradient(int ix, int iy) {
//     const uint w = 32u;            // 32-bit uint in GLSL
//     const uint s = w / 2u;         // rotation width
//     uint a = uint(ix), b = uint(iy);

//     a *= 3284157443u;              
//     b ^= (a << s) | (a >> (w - s));  // bitwise rotation
//     b *= 1911520717u;
//     a ^= (b << s) | (b >> (w - s));
//     a *= 2048419325u;

//     // Convert `a` to a float in range [0, 2*Pi]
//     float random = float(a) * (3.14159265 / float(0x7FFFFFFF));

//     // Return the gradient vector
//     return vec2(cos(random), sin(random));
// }

// // Computes the dot product of the distance and gradient vectors.
// float dotGridGradient(int ix, int iy, float x, float y) {
//     // Get gradient from integer coordinates
//     vec2 gradient = randomGradient(ix, iy);

//     // Compute the distance vector
//     float dx = x - float(ix);
//     float dy = y - float(iy);

//     // Compute the dot-product
//     return (dx*gradient.x + dy*gradient.y);
// }

// // Compute Perlin noise at coordinates x, y
// float perlin(float x, float y) {
//     // Determine grid cell coordinates
//     int x0 = int(floor(x));
//     int x1 = x0 + 1;
//     int y0 = int(floor(y));
//     int y1 = y0 + 1;

//     // Determine interpolation weights
//     // Could also use higher order polynomial/s-curve here
//     float sx = x - float(x0);
//     float sy = y - float(y0);

//     // Interpolate between grid point gradients
//     float n0, n1, ix0, ix1, value;

//     n0 = dotGridGradient(x0, y0, x, y);
//     n1 = dotGridGradient(x1, y0, x, y);
//     ix0 = interpolate(n0, n1, sx);

//     n0 = dotGridGradient(x0, y1, x, y);
//     n1 = dotGridGradient(x1, y1, x, y);
//     ix1 = interpolate(n0, n1, sx);

//     value = interpolate(ix0, ix1, sy);
//     return value; // Will return in range -1 to 1. To make it in range 0 to 1, multiply by 0.5 and add 0.5
// }

// float fractalBrownianMotion(float i, float j, int numOctaves, float scale, float persistance, float lacunarity){
//     float amplitude = 1.0f;
//     float frequency = 1.0f;
//     float noiseHeight = 0.0f;

//     if(scale <= 0.0) scale = 0.00001f;

//     for(int k = 0; k < numOctaves; k++){
//         float sampleX = i / scale * frequency;
//         float sampleY = j / scale * frequency;
//         float perlinValue = perlin(sampleX, sampleY) * 2 - 1;
//         noiseHeight += perlinValue * amplitude;
//         amplitude *= persistance;
//         frequency *= lacunarity; 
//     }

//     return noiseHeight;
// }

// float inverseLerp(float a, float b, float value) {
//     if (a != b) { // Evita la divisione per zero
//         return (value - a) / (b - a);
//     } else {
//         return 0.0f; // Se a e b sono uguali, restituisce 0
//     }
// }

void main()
{
	// get patch coordinate
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	// ----------------------------------------------------------------------
	// retrieve control point texture coordinates
	vec2 t00 = TextureCoord[0];
	vec2 t01 = TextureCoord[1];
	vec2 t10 = TextureCoord[2];
	vec2 t11 = TextureCoord[3];

	// bilinearly interpolate texture coordinate across patch
	vec2 t0 = (t01 - t00) * u + t00;
	vec2 t1 = (t11 - t10) * u + t10;
	texCoord = (t1 - t0) * v + t0;

	// lookup texel at patch coordinate for height and scale + shift as desired
	Height = texture(heightMap, texCoord).x * 8.0;

	// ----------------------------------------------------------------------
	// retrieve control point position coordinates
	vec4 p00 = gl_in[0].gl_Position;
	vec4 p01 = gl_in[1].gl_Position;
	vec4 p10 = gl_in[2].gl_Position;
	vec4 p11 = gl_in[3].gl_Position;

	// compute patch surface normal
	vec4 uVec = p01 - p00;
	vec4 vVec = p10 - p00;
	vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );

	// bilinearly interpolate position coordinate across patch
	vec4 p0 = (p01 - p00) * u + p00;
	vec4 p1 = (p11 - p10) * u + p10;
	vec4 p = (p1 - p0) * v + p0;

	//Height = fractalBrownianMotion(p.x, p.z, octaves, 20.3, 0.5, 2.0);
	// displace point along normal
	p += normal * Height;

	// ----------------------------------------------------------------------
	// output patch point position in clip space
	gl_Position = mvpMatrix * p;
}