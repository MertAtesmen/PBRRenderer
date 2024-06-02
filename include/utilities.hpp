
#include <iostream>

#include <glm/glm.hpp>
#include "glad/glad.h"
#include <vector>

#ifndef UTILS_HPP
#define UTILS_HPP

std::ostream& operator<<(std::ostream& out, const glm::vec3& vec){
    out << "(" << vec.x <<", " << vec.y << ", " << vec.z << ")";
    return out;  
}

std::ostream& operator<<(std::ostream& out, const glm::vec4& vec){
    out << "(" << vec.x <<", " << vec.y << ", " << vec.z << ", " <<  vec.w << ")";
    return out;  
}

namespace PBR{
    void gl_clear_errors(){
        while(glGetError() != GL_NO_ERROR);
    }

    const char* gl_src_name(GLenum source){
        switch (source)
        {
        case GL_DEBUG_SOURCE_API:
            return "Open GL";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "Third Party";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "Application User";
        case GL_DEBUG_SOURCE_OTHER:
        default:
            return "Other";
        }
    }

    const char* gl_type_name(GLenum type){
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            return "API";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "Deprecated";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "Undefined Behaviour";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "Performance Issue";
        case GL_DEBUG_TYPE_MARKER:
            return "Command Stream";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "Group Pushing";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "Group Popping";
        case GL_DEBUG_TYPE_OTHER:
        default:
            return "Other";
        }
    }   

    const char* gl_severity(GLenum severity){
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            return "High";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "Medium";
        case GL_DEBUG_SEVERITY_LOW:
            return "Low";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "Notification";
        default:
            return "Unknown";
        }
    }

    void GLAPIENTRY MessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
        std::cerr << "GL CALLBACK: source = " << gl_src_name(source)
            << ", type = " << gl_type_name(type)
            << ", severity = " << gl_severity(severity)
            << ", message = " << message        
            << '\n';
    }
}

#endif