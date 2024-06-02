#Vertex Shader

#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCord;
 
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main(){
    vec4 vertexLocation = model * vec4(aPos, 1.0f);
    gl_Position = projection * view * vertexLocation;
    FragPos = vec3(vertexLocation);
    Normal = normalMatrix * aNormal;
    TexCord = aTexCord;
}

#Fragment Shader

#version 460 core

#define PI 3.1415926535897932384626433832795


out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCord;

uniform vec3 color;
uniform float roughness;
uniform float metallic;

uniform samplerCube irradiance_map;
uniform samplerCube prefilter_map;
uniform sampler2D brdfLUT;

// Light struct 
struct Light{
    vec3 position;    
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 attenuationScalars;

    bool isDirLight;
};

uniform vec3 lightDirections[4];


uniform Light light;
uniform vec3 viewPos;

float RadicalInverse_VdC(uint bits);
vec2 Hammersley(uint i, uint N);
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec2 IntegrateBRDF(float NdotV, float roughness);

vec3 schlickFresnel(float vDotH, float metallic, vec3 color);
vec3 fresnelSchlickRoughness(float nDotV, float metallic, vec3 color, float roughness);
float geomSmith(float dp, float roughness);
float ggxDistribution(float nDotH, float roughness);

vec3 calcPBRLighting(Light light, vec3 posDir, bool isDirlight, float metallic, vec3 color, vec3 normal, float roughness);
vec3 calcPBRAmbientLightning(float metallic, vec3 color, vec3 normal, float roughness, float ao);

void main()
{    
    vec3 albedo = pow(color, vec3(2.2));

    vec3 normal = normalize(Normal);
    vec3 view_dir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0f);

    for(int i = 0; i < 4; ++i){
        result += calcPBRLighting(light, normalize(lightDirections[i]), light.isDirLight, metallic, albedo, normal, roughness);
    }

    result += calcPBRAmbientLightning(metallic, albedo, normal, roughness, 0.03);

    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0/2.2)); 


    FragColor = vec4(result, 1.0f);
}


vec3 schlickFresnel(float vDotH, float metallic, vec3 color)
{
    vec3 F0 = vec3(0.04);

    F0 = mix(F0, color, metallic);

    vec3 ret = F0 + (1 - F0) * pow(clamp(1.0 - vDotH, 0.0, 1.0), 5);

    return ret;
}

vec3 fresnelSchlickRoughness(float nDotV, float metallic, vec3 color, float roughness)
{
    vec3 F0 = vec3(0.04);

    F0 = mix(F0, color, metallic);

    vec3 ret = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - nDotV, 0.0, 1.0), 5.0);

    return ret;
}   


float geomSmith(float dp, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float denom = dp * (1.0 - k) + k;
    return dp / denom;
}


float ggxDistribution(float nDotH, float roughness)
{
    float alpha2 = roughness * roughness * roughness * roughness;
    float d = nDotH * nDotH * (alpha2 - 1.0f) + 1.0f;
    float ggxdistrib = alpha2 / (PI * d * d);
    return ggxdistrib;
}


vec3 calcPBRLighting(Light light, vec3 posDir, bool isDirlight, float metallic, vec3 color, vec3 normal, float roughness)
{
    vec3 lightIntensity = light.diffuse;

    // Light Direction vector
    vec3 L = vec3(0.0);

    if (isDirlight) {
        L = -posDir.xyz;
    } else {
        L = posDir - FragPos;
        float lightToPixelDist = length(L);
        L = normalize(L);
        lightIntensity /= (lightToPixelDist * lightToPixelDist);
    }

    // Normal vector
    vec3 N = normal;

    // View direction vector
    vec3 V = normalize(viewPos - FragPos);

    // Halfway direction vector
    vec3 H = normalize(V + L);

    float nDotH = max(dot(N, H), 0.0);
    float vDotH = max(dot(V, H), 0.0);
    float nDotL = max(dot(N, L), 0.0);
    float nDotV = max(dot(N, V), 0.0);

    vec3 F = schlickFresnel(vDotH, metallic, color);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;

    kD *= 1.0 - metallic;

    vec3 SpecBRDF_nom  = ggxDistribution(nDotH, roughness) *
                         F *
                         geomSmith(nDotL, roughness) *
                         geomSmith(nDotV, roughness);

    float SpecBRDF_denom = 4.0 * nDotV * nDotL + 0.0001;

    vec3 SpecBRDF = SpecBRDF_nom / SpecBRDF_denom;

    vec3 DiffuseBRDF = kD * color / PI;

    vec3 FinalColor = (DiffuseBRDF + SpecBRDF) * lightIntensity * nDotL;

    return FinalColor;
}



vec3 calcPBRAmbientLightning(float metallic, vec3 color, vec3 normal, float roughness, float ao)
{    
    vec3 N = normal;

    vec3 V = normalize(viewPos - FragPos);

    vec3 R = reflect(-V, N); 

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), metallic, color,  roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradiance_map, N).rgb;
    vec3 diffuse = irradiance * color;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefiltered_color = textureLod(prefilter_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    // vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec2 brdf  = IntegrateBRDF(max(dot(N, V), 0.0), roughness);
    vec3 specular = prefiltered_color * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;

    return ambient;
}


float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0; 

    vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 32u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}