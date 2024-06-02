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

// Maps for the PBR 
uniform sampler2D albedo_map;
uniform sampler2D ao_map;
uniform sampler2D metallic_map;
uniform sampler2D normal_map;
uniform sampler2D roughness_map;
uniform sampler2D change_map;
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

vec3 getNormalFromMap();

vec3 schlickFresnel(float vDotH, bool isMetal, vec3 color);
float geomSmith(float dp, float roughness);
float ggxDistribution(float nDotH, float roughness);

vec3 calcPBRLighting(Light light, vec3 posDir, bool isDirlight, bool isMetal, vec3 color, vec3 normal, float roughness);


void main()
{    
    vec3 albedo = vec3(texture(albedo_map, TexCord));
    float ambient_occlision = texture(ao_map, TexCord).r;
    float metallic = texture(metallic_map, TexCord).r;
    float roughness = texture(roughness_map, TexCord).r;

    bool isMetal = metallic > 0.5f;

    vec3 normal = getNormalFromMap();
    vec3 view_dir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0f);

    for(int i = 0; i < 4; ++i){
        result += calcPBRLighting(light, normalize(lightDirections[i]), light.isDirLight, isMetal, albedo, Normal, roughness);
    }

    vec3 ambient = vec3(0.01) * albedo * ambient_occlision;


    FragColor = vec4(result + ambient, 1.0f);
}


vec3 schlickFresnel(float vDotH, bool IsMetal, vec3 color)
{
    vec3 F0 = vec3(0.04);

    if (IsMetal) {
        F0 = color;
    }

    vec3 ret = F0 + (1 - F0) * pow(clamp(1.0 - vDotH, 0.0, 1.0), 5);

    return ret;
}


float geomSmith(float dp, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float denom = dp * (1 - k) + k;
    return dp / denom;
}


float ggxDistribution(float nDotH, float roughness)
{
    float alpha2 = roughness * roughness * roughness * roughness;
    float d = nDotH * nDotH * (alpha2 - 1) + 1;
    float ggxdistrib = alpha2 / (PI * d * d);
    return ggxdistrib;
}


vec3 calcPBRLighting(Light light, vec3 posDir, bool isDirlight, bool isMetal, vec3 color, vec3 normal, float roughness)
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

    vec3 F = schlickFresnel(vDotH, isMetal, color);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;

    vec3 SpecBRDF_nom  = ggxDistribution(nDotH, roughness) *
                         F *
                         geomSmith(nDotL, roughness) *
                         geomSmith(nDotV, roughness);

    float SpecBRDF_denom = 4.0 * nDotV * nDotL + 0.0001;

    vec3 SpecBRDF = SpecBRDF_nom / SpecBRDF_denom;

    vec3 fLambert = vec3(0.0);

    if (!isMetal) {
        fLambert = color;
    }

    vec3 DiffuseBRDF = kD * fLambert / PI;

    vec3 FinalColor = (DiffuseBRDF + SpecBRDF) * lightIntensity * nDotL;

    return FinalColor;
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normal_map, TexCord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCord);
    vec2 st2 = dFdy(TexCord);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}