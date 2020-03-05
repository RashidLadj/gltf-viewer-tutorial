#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;

uniform sampler2D uBaseColorTexture;
uniform vec4 uBaseColorFactor;
uniform float uMetallicFactor; 
uniform float uRoughnessFactor; 
uniform sampler2D uMetallicRoughnessTexture;

uniform vec3 uEmissiveFactor; 
uniform sampler2D uEmissiveTexture;

out vec3 fColor;

// Constants
const float GAMMA = 2.2;
const float INV_GAMMA = 1. / GAMMA;
const float M_PI = 3.141592653589793;
const float M_1_PI = 1.0 / M_PI;
const vec3 dielectricSpecular = vec3(0.04, 0.04, 0.04);
const vec3 black = vec3(0, 0, 0);

// We need some simple tone mapping functions
// Basic gamma = 2.2 implementation
// Stolen here: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/tonemapping.glsl

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 LINEARtoSRGB(vec3 color){
  return pow(color, vec3(INV_GAMMA));
}

// sRGB to linear approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec4 SRGBtoLINEAR(vec4 srgbIn){
  return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w);
}

void main(){
    vec3 N = normalize(vViewSpaceNormal);
    vec3 L = uLightDirection; 
    vec3 V = normalize(-vViewSpacePosition);
    vec3 H = normalize(L + V);


    vec4 baseColorFromTexture = SRGBtoLINEAR(texture(uBaseColorTexture, vTexCoords));
    vec4 baseColor = baseColorFromTexture * uBaseColorFactor;
    vec4 metallicRougnessFromTexture = texture(uMetallicRoughnessTexture, vTexCoords);
    float roughness = uRoughnessFactor * metallicRougnessFromTexture.g;
    vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);

    vec3 cDiff = mix(baseColor.rgb * (1 - dielectricSpecular.r), black, metallic);
    vec3 F0 = mix(vec3(dielectricSpecular), baseColor.rgb, metallic);
    float _alpha = roughness * roughness;
    float _alphaPow2 = _alpha * _alpha;
    

    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float VdotH = clamp(dot(V, H), 0.0, 1.0);

    vec3 diffuse = cDiff * M_1_PI;

    
    /** F **/
    float baseShlickFactor = 1 - VdotH;
    float shlickFactor = baseShlickFactor * baseShlickFactor; // power 2
    shlickFactor *= shlickFactor; // power 4
    shlickFactor *= baseShlickFactor; // power 5
    vec3 F = F0 + ( 1 - F0) * shlickFactor;

    /** Vis **/
    float Vis = 0;
    float denumVis = (NdotL) *sqrt((NdotV*NdotV) * (1 - _alphaPow2) + _alphaPow2) + (NdotV) * sqrt((NdotL*NdotL) *  (1 - _alphaPow2) + _alphaPow2);
    if (denumVis > 0){
        Vis =  0.5 / denumVis;
    }

    /** D **/
    float DenumD = M_PI * ((NdotH*NdotH * (_alphaPow2 - 1) + 1) * (NdotH*NdotH * (_alphaPow2 - 1) + 1));
    float D = _alphaPow2 / DenumD;

    /** f_specular = F . Vis . D **/
    vec3 f_specular = F * Vis * D;


    vec3 f_diffuse = (1 - F) * diffuse;

    vec4 emissiveRougnessFromTexture = SRGBtoLINEAR(texture(uEmissiveTexture, vTexCoords));
    vec3 emissive = uEmissiveFactor * emissiveRougnessFromTexture.rgb;

    fColor = LINEARtoSRGB((f_diffuse + f_specular) * uLightIntensity * NdotL ) ;

}
