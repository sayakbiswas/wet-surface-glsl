#version 330 core

struct V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
};

in vec3 position_worldspace;
in vec3 normal_cameraspace;
in vec3 eyeDirection_cameraspace;
in vec3 lightDirection_cameraspace;
in V2F vdata;

out vec3 color;

uniform vec3 lightPosition_worldspace;

float basicNoise(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

//3D Value Noise generator by Morgan McGuire @morgan3d
//https://www.shadertoy.com/view/4dS3Wd
float hash(float n) { return fract(sin(n) * 1e4); }
float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(vec3 x) {
	const vec3 step = vec3(110, 241, 171);

	vec3 i = floor(x);
	vec3 f = fract(x);

	// For performance, compute the base input to a 1D hash from the integer part of the argument and the
	// incremental change to the 1D based on the 3D -> 1D wrapping
    float n = dot(i, step);

	vec3 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix( hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x),
		   mix( hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y),
	       mix(mix( hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x),
		   mix( hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z);
}

//Fractional Brownian Motion
#define NUM_OCTAVES 2

float fnoise(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100);
	for (int i = 0; i < NUM_OCTAVES; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}


void main() {
    vec3 lightColor = vec3(1, 1, 1);
    float lightPower = 20.0f;

    vec3 materialDiffuseColor = vdata.color.rgb;
    vec3 materialAmbientColor = vec3(0.3, 0.3, 0.3) * materialDiffuseColor;
    vec3 materialSpecularColor = vec3(0.5, 0.5, 0.5) * materialDiffuseColor;

    float distance = length(lightPosition_worldspace - position_worldspace);

    vec3 n = normalize(normal_cameraspace);
    vec3 l = normalize(lightDirection_cameraspace);

    //Generating a bump map from noise
    float inz = fnoise(vdata.position) * 0.5 + 0.5;
    float E = 0.001;

    vec3 px = vdata.position;
    px.x += E;
    vec3 py = vdata.position;
    py.y += E;
    vec3 pz = vdata.position;
    pz.z += E;

    vec3 bump = vec3(fnoise(px)*0.5+0.5, fnoise(py)*0.5+0.5, fnoise(pz)*0.5+0.5);
    vec3 pN = vec3((bump.x-inz)/E, (bump.y-inz)/E, (bump.z-inz)/E);

    n = normalize(n - pN);

    float cosTheta = clamp(dot(n, l), 0, 1);

    vec3 e = normalize(eyeDirection_cameraspace);
    vec3 r = reflect(-l, n);

    float cosAlpha = clamp(dot(e,r), 0, 1);

    float F0 = 0.5;
    vec3 h = normalize(e+l);
    float base = 1 - dot(e, h);
    float exponential = pow(base, 5.0);
    float fresnel = exponential + F0 * (1.0 - exponential);


    color = materialAmbientColor
	    + (materialDiffuseColor + lightColor) * lightPower * cosTheta / (distance * distance)
	    + ((materialSpecularColor + lightColor) * basicNoise(vdata.position.xy) * 20
		* lightPower * pow(max(0.0, cosAlpha), 200) / (distance * distance))
		* fresnel;

    color = clamp(color, 0., 1.);
}
