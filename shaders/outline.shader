#Vertex Shader

#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCord;
 
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    vec4 vertexLocation = model * vec4(aPos, 1.0f);
    gl_Position = projection * view * vertexLocation;
}

#Fragment Shader

#version 460 core
out vec4 FragColor;

uniform vec3 outlineColor;

void main()
{    
    FragColor = vec4(outlineColor, 1.0f);
}
