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

void main() {
    vec3 lightColor = vec3(1, 1, 1);
    float lightPower = 70.0f;

    vec3 materialDiffuseColor = vdata.color.rgb;
    vec3 materialAmbientColor = vec3(0.3, 0.3, 0.3) * materialDiffuseColor;
    vec3 materialSpecularColor = vec3(0.5, 0.5, 0.5) * materialDiffuseColor;

    float distance = length(lightPosition_worldspace - position_worldspace);

    vec3 n = normalize(normal_cameraspace);
    vec3 l = normalize(lightDirection_cameraspace);

    float cosTheta = clamp(dot(n, l), 0, 1);

    vec3 e = normalize(eyeDirection_cameraspace);
    vec3 r = reflect(-l, n);

    float cosAlpha = clamp(dot(e,r), 0, 1);

    color = materialAmbientColor
	    + materialDiffuseColor * lightColor * lightPower * cosTheta / (distance * distance)
	    + materialSpecularColor * lightColor * lightPower * pow(max(0.0, cosAlpha), 50) / (distance * distance);
}
