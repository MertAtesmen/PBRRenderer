#pragma once

// #define DEBUG_MESSAGES_ENABLED 
// Enable Debug messages

// #define BUFFER_CLEAN_DISABLED
// Disable Cleaning

#include "config.hpp"
#include "utilities.hpp"

#include <vector>
#include <iostream>

namespace PBR{
    class ArrayBuffer
    {
    public:
        ArrayBuffer(const std::vector<float>& vertices, const std::vector<VertexAttribConfig>& _configs);
		
		ArrayBuffer(ArrayBuffer&& other);
		ArrayBuffer& operator=(ArrayBuffer&& other);

		
		ArrayBuffer(const ArrayBuffer&) = delete;
		auto operator=(const ArrayBuffer&) = delete;		
    
        bool bind() const;
        bool unbind() const;

        int get_ID();
        #ifndef BUFFER_CLEAN_DISABLED
        ~ArrayBuffer();
        #endif
    private:
        int ID{ -1 };
        std::vector<VertexAttribConfig> configs{ };

        void create_vbo();

        bool fill_buffer(const std::vector<float>& vertices);
    };
}

namespace PBR{
    ArrayBuffer::ArrayBuffer(const std::vector<float>& vertices, const std::vector<VertexAttribConfig>& _configs)
            : configs{ _configs }
        {   
            // Bind 0 before construction to prevent modifying the currently bound Vertex Array Object
            glBindVertexArray(0);
            
            create_vbo();

            if(!fill_buffer(vertices)){
                #ifdef DEBUG_MESSAGES_ENABLED
                    std::cerr << "[GL] ERROR in glBufferData: Failed to fill the Vertex Buffer " << std::endl;
                #endif
                return;
            }
            #ifdef DEBUG_MESSAGES_ENABLED
                std::cout << "[GL] Succesfully Created Vertex Buffer Object\n" << std::endl;
            #endif
        }

		ArrayBuffer::ArrayBuffer(ArrayBuffer&& other): ID{other.ID}{
            other.ID = -1;
		}
	    ArrayBuffer& ArrayBuffer::operator=(ArrayBuffer&& other){
            this->ID = other.ID;
            other.ID = -1;
            return *this;
        }

        bool ArrayBuffer::bind() const{
            // This function will bind the vertex buffer to the currently bound Vertex Array Object  
            if(this->ID == -1) return false;

            glBindBuffer(GL_ARRAY_BUFFER, this->ID);

            // Enable all the attributes
            for(const VertexAttribConfig& config: configs){
                glVertexAttribPointer(
                    config.index, 
                    config.size, 
                    config.type, 
                    config.normalized, 
                    config.stride, 
                    reinterpret_cast<void *>(config.offset)
                );
                glEnableVertexAttribArray(config.index);
            }

            return true;
        }

        bool ArrayBuffer::unbind() const{
            if(this->ID == -1) return false;
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return true;
        }

        int ArrayBuffer::get_ID(){
             return this->ID;
        }

        #ifndef BUFFER_CLEAN_DISABLED
        ArrayBuffer::~ArrayBuffer(){
            if(this->ID != -1){
                unsigned int buffer = static_cast<unsigned int>(ID);
                glDeleteBuffers(1, &buffer);
            }
        }
        #endif

        void ArrayBuffer::create_vbo(){
            unsigned int VBO;
            // This function should always succeed as if not then there is no buffer left to assign
            glGenBuffers(1, &VBO); 
            this->ID = static_cast<int>(VBO);
        }

        bool ArrayBuffer::fill_buffer(const std::vector<float>& vertices){
            // if(this->ID == -1) return false;
            // This function should always succeed since there are no invalid arguments
            glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(this->ID));
            
            PBR::gl_clear_errors();
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            GLenum err = glGetError();

            #ifdef DEBUG_MESSAGES_ENABLED
                if(err != GL_NO_ERROR) std::cerr << "[GL] ERROR:" << glGetString(err) << std::endl;
            #endif

            return err == GL_NO_ERROR;
        }
}