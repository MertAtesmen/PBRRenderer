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
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCord;

uniform sampler2D albedo_map;
uniform sampler2D ao_map;
uniform sampler2D metallic_map;
uniform sampler2D normal_map;
uniform sampler2D roughness_map;
uniform sampler2D change_map;


struct DirectionalLight{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirectionalLight directional_light;
uniform vec3 viewPos;


vec3 calculateDirectionalLight(DirectionalLight light, vec3 object_color, vec3 specular_color, vec3 normal, vec3 view_dir);

void main()
{    
    vec3 albedo = vec3(texture(albedo_map, TexCord));
    float ambient_occlision = (texture(ao_map, TexCord)).r;
    float metallic = (texture(metallic_map, TexCord)).r;
    float roughness = (texture(roughness_map, TexCord)).r;
    vec3 change = vec3(texture(change_map, TexCord));


    vec3 normal = normalize(Normal);
    vec3 view_dir = normalize(viewPos - FragPos);

    // vec3 result = calculateDirectionalLight(directional_light, change, change, normal, view_dir);
    vec3 result = change;

    FragColor = vec4(result, 1.0f);
}

vec3 calculateDirectionalLight(DirectionalLight light, vec3 object_color, vec3 specular_color, vec3 normal, vec3 view_dir){
    //Calculate variables
    vec3 light_dir = normalize(-light.direction); 
    vec3 halfway_dir = normalize(light_dir + view_dir);

    // Ambinet
    vec3 ambient_lightning =  light.ambient * object_color;
    
    // Diffuse
    float diffuseStrength = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse_lightning = diffuseStrength * light.diffuse * object_color;
    
    // Specular
    float spec = pow(max(dot(normal, halfway_dir), 0.0f), 32.0f);
    vec3 specular_lightning = spec * light.specular * specular_color;

    // Final results
    vec3 result = ambient_lightning + specular_lightning + diffuse_lightning;

    return result;
}