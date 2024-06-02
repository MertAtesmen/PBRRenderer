#include "ArrayBuffer.hpp"
#include "ElementBuffer.hpp"


#include <glad/glad.h>

#include <iostream>

// #define DEBUG_MESSAGES_ENABLED 
// Enable Debug messages

#pragma once

namespace PBR{
    class VertexArrayObject
    {
        /* data */
    public:
        VertexArrayObject() {
            if(!create_vao()){
                #ifdef DEBUG_MESSAGES_ENABLED
                    std::cerr << "ERROR in glGenVertexArrays: Failed to generate Vertex Array Object" << std::endl;
                #endif
                return;
            }
        }

        bool bind(const ArrayBuffer& vertex_buffer){
            // Bind the Vertex Array Object before binding the Vertex Buffer 
            if(!enable()) return false;
            if(!vertex_buffer.bind()) return false;
            // Unbind the Vertex Array Object
            return disable();
        }

        bool bind(const ElementBuffer& index_buffer){
            // Bind the Vertex Array Object before binding the Vertex Buffer 
            if(!enable()) return false;
            if(!index_buffer.bind()) return false;
            // Unbind the Vertex Array Object
            return disable();
        }

        bool enable(){
            if(ID == -1) return false;
            glBindVertexArray(this->ID);
            return true;
        }

        bool disable(){
            if(ID == -1) return false;
            glBindVertexArray(0);
            return true;
        }

        ~VertexArrayObject() {}
    private:

        int ID{ -1 };

        bool create_vao(){
            unsigned int VAO;
            glGenVertexArrays(1, &VAO);
            // Add more debugging
            if(VAO == 0){

                return false;
            }
            ID = VAO;
            return true;
        }



    };
}