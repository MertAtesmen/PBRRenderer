#Vertex Shader

#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCord;
 
out vec2 TexCord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    gl_Position = projection * view * model * vec4(aPos, 1.0f);;
    TexCord = aTexCord;
}

#Fragment Shader

#version 460 core

out vec4 FragColor;

in vec2 TexCord;

uniform sampler2D texture1;

void main()
{    
    FragColor = vec4(texture(texture1, TexCord).rgb, 1.0f);
}
