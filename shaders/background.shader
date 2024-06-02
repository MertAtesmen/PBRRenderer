#Vertex Shader

#version 460 core

layout (location = 0) in vec3 aPos;

out vec3 WorldPos;


uniform mat4 projection;
uniform mat4 view;


void main()
{
    WorldPos = aPos;

    mat4 clear_view = mat4(mat3(view));
	vec4 clipPos = projection * clear_view * vec4(WorldPos, 1.0);

	gl_Position = clipPos.xyww;
}


#Fragment Shader

#version 460 core

in vec3 WorldPos;

out vec4 FragColor;

uniform samplerCube environmentMap;

void main()
{		
    vec3 envColor = texture(environmentMap, WorldPos).rgb;
    
    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
}
