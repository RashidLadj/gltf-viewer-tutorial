#version 330

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec3 vViewSpacePosition;
out vec3 vViewSpaceNormal;
out vec2 vTexCoords;
out mat3 TBN;

out vec3 vTangent;


uniform mat4 uModelMatrix;
uniform mat4 uModelViewProjMatrix;
uniform mat4 uModelViewMatrix;
uniform mat4 uNormalMatrix;

void main()
{
    vViewSpacePosition = vec3(uModelViewMatrix * vec4(aPosition, 1.0));
	vViewSpaceNormal = normalize(vec3(uNormalMatrix * vec4(aNormal, 0.0)));
	vTexCoords = aTexCoords;

    // On multiplie par la modelMatrix car on veut uniquement leur orientation dans le "tangent space",
    //si on voulait aussi leur directino il faudrait multiplier en plus par la normal matrix
    //vec3 T = normalize(vec3(uModelMatrix * vec4(aTangent, 0.0)));
    //vec3 B = normalize(vec3(uModelMatrix * vec4(aBitangent, 0.0)));
    //vec3 N = normalize(vec3(uModelMatrix * vec4(aNormal, 0.0)));
    //TBN = mat3(T, B, N);
    //vTangent = T;

    vTangent = aTangent;

    gl_Position =  uModelViewProjMatrix * vec4(aPosition, 1.0);
}