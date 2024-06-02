#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string_view>
#include <filesystem>
#include <vector>


namespace PBR{

    enum class ShaderType{
        FragmentShader = GL_FRAGMENT_SHADER,
        VertexShader = GL_VERTEX_SHADER,
        Program,
        None
    };

    const char* shaderTypeToString(ShaderType type){
        switch (type)
        {
        case ShaderType::VertexShader:
            return "Vertex Shader";
        case ShaderType::FragmentShader:
            return "Fragment Shader";
        case ShaderType::Program:
            return "Program";
        case ShaderType::None: 
            return "None";
        default:
            return "Unkonwn";
            break;
        }
    }

    struct ShaderInfo{
        std::string shaderSource;
        ShaderType type;
    };

    class Shader
    {
    public:
        unsigned int ID;
        // constructor generates the shader on the fly
        // ------------------------------------------------------------------------
        Shader() = default;
        Shader(std::string_view shaderPath)
        {
            try 
            {
                std::ifstream shaderFile{ shaderPath.data() };
                
                auto shaders = Shader::readShaderFile(shaderFile);
                ID = Shader::compileShader(shaders);

            }
            catch (std::ifstream::failure& e)
            {
                std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << shaderPath << ", " << e.what() << '\n';
            }
            catch (std::exception& e)
            {
                std::cerr << "ERROR::SHADER::FAILURE_IN_SHADER: " << e.what() << '\n';
            }

        }
        // activate the shader
        // ------------------------------------------------------------------------
        void use() const
        { 
            glUseProgram(ID); 
        }
        // utility uniform functions
        // ------------------------------------------------------------------------
        void setBool(const std::string &name, bool value) const
        {         
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform1i(location, (int)value); 
        }
        // ------------------------------------------------------------------------
        void setInt(const std::string &name, int value) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform1i(location, value); 
        }
        // ------------------------------------------------------------------------
        void setFloat(const std::string &name, float value) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform1f(location, value); 
        }
        // ------------------------------------------------------------------------
        void setVec2(const std::string &name, const glm::vec2 &value) const
        {         // // shader Program

            int location = glGetUniformLocation(ID, name.c_str());
            glUniform2fv(location, 1, &value[0]); 
        }
        void setVec2(const std::string &name, float x, float y) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform2f(location, x, y); 
        }
        // ------------------------------------------------------------------------
        void setVec3(const std::string &name, const glm::vec3 &value) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform3fv(location, 1, &value[0]); 
        }
        void setVec3(const std::string &name, float x, float y, float z) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform3f(location, x, y, z); 
        }
        // ------------------------------------------------------------------------
        void setVec4(const std::string &name, const glm::vec4 &value) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform4fv(location, 1, &value[0]); 
        }
        void setVec4(const std::string &name, float x, float y, float z, float w) const
        { 
            int location = glGetUniformLocation(ID, name.c_str());
            glUniform4f(location, x, y, z, w); 
        }
        // ------------------------------------------------------------------------
        void setMat2(const std::string &name, const glm::mat2 &mat) const
        {
            int location = glGetUniformLocation(ID, name.c_str());
            glUniformMatrix2fv(location, 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat3(const std::string &name, const glm::mat3 &mat) const
        {
            int location = glGetUniformLocation(ID, name.c_str());
            glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat4(const std::string &name, const glm::mat4 &mat) const
        {
            int location = glGetUniformLocation(ID, name.c_str());
            glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
        }

    private:
        // utility function for checking shader compilation/linking errors.
        // ------------------------------------------------------------------------
        bool checkCompileErrors(GLuint shader, ShaderType type)
        {
            GLint success;
            GLchar infoLog[1024];
            if (type != ShaderType::Program)
            {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: Shader\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                    return true;
                }
            }
            else // This is a program
            {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if (!success)
                {
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: Program\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                    return true;
                }
            }
            return false;
        }
        std::vector<ShaderInfo> readShaderFile(std::ifstream& file){
            std::vector<ShaderInfo> shaders;

            bool isFirst = true;
            std::stringstream stream;
            ShaderType lastType = ShaderType::None;
            std::string line;
            while (std::getline(file, line)){
                // std::cout  << "Read the line: " << line << "\n";
                if(line.find("#") != std::string::npos && line.find("Shader") != std::string::npos){
                    if(!isFirst){
                        // std::cout << "Pushing back the value" << stream.str() << "\n"; 
                        shaders.push_back(ShaderInfo{stream.str(), lastType});
                    }
                    stream = std::stringstream{ }; // Clear the buffer;
                    lastType = Shader::__get_shader_type(line);
                    isFirst = false;

                }
                else{
                    stream << line << "\n";
                }
            }
            if(stream.tellp() != std::streampos(0)){
                // std::cout << "Pushing back the value" << stream.str() << "\n"; 
                shaders.push_back(ShaderInfo{stream.str(), lastType});
            }

            return shaders;
        }

        unsigned int compileShader(const std::vector<ShaderInfo>& shaders){
            std::vector<unsigned int> shaderIDs;
            for(const ShaderInfo& info: shaders){
                shaderIDs.push_back(Shader::__compile_shader(info));
            }
            unsigned int programID = Shader::__compile_program(shaderIDs);
            for (unsigned int id: shaderIDs){
                glDeleteShader(id);
            }
            return programID;
        }

        ShaderType __get_shader_type(std::string_view typeString){
            if(typeString.find("Vertex Shader") != std::string::npos){
                return ShaderType::VertexShader;
            }
            else if(typeString.find("Fragment Shader") != std::string::npos){
                return ShaderType::FragmentShader;
            }
            return ShaderType::None;
        }

        unsigned int __compile_shader(const ShaderInfo& info){
            const char* src = info.shaderSource.c_str();
            // std::cout << "\n\nShader Info:\n"
            //     << "Type: " << PBR::shaderTypeToString(info.type) << '\n'
            //     << "Src: " << src << '\n';
            unsigned int shaderID = glCreateShader(static_cast<GLenum>(info.type));
            glShaderSource(shaderID, 1, &src, nullptr);
            glCompileShader(shaderID);
            if(Shader::checkCompileErrors(shaderID, info.type))
                throw std::runtime_error{ "Shader Complation Error" };

            return shaderID;
        }

        unsigned int __compile_program(std::vector<unsigned int> shaderIDs){
            unsigned int programID = glCreateProgram();
            for(unsigned int handle: shaderIDs){
                glAttachShader(programID, handle);
            }
            glLinkProgram(programID);
            if(Shader::checkCompileErrors(programID, ShaderType::Program))
                throw std::runtime_error{ "Program Complation Error" };
            return programID;
        }

    };


}