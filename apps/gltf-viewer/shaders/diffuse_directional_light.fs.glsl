#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;

out vec3 fColor;

float Pi = 1. / 3.14;

void main() {
    fColor= vec3(Pi) * uLightIntensity * dot(normalize(vViewSpaceNormal), uLightDirection);
}	