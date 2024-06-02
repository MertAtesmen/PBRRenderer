#pragma once

// #define DEBUG_MESSAGES_ENABLED 
// Enable Debug messages

// #define BUFFER_CLEAN_DISABLED
// Disable Cleaning


#include <vector>
#include <iostream>

#include <glad/glad.h>

namespace PBR{
    class ElementBuffer
    {
    public:
        ElementBuffer(const std::vector<unsigned int>& indices);

        ElementBuffer(ElementBuffer&& other);
        ElementBuffer& operator=(ElementBuffer&& other);

        ElementBuffer() = delete;
        ElementBuffer(const ElementBuffer&)= delete;
        auto operator=(const ElementBuffer& )= delete;

        bool bind() const;
        bool unbind() const;

        int get_ID();


        #ifndef BUFFER_CLEAN_DISABLED
        ~ElementBuffer();
        #endif
    private:
        int ID{ -1 };

        bool create_ebo();
        bool fill_buffer(const std::vector<unsigned int>& indices);
    };
}



namespace PBR{
    ElementBuffer::ElementBuffer(const std::vector<unsigned int>& indices) 
    {
        // Bind 0 before construction to prevent modifying the currently bound Vertex Array Object
        glBindVertexArray(0);
        if(!create_ebo()){
            #ifdef DEBUG_MESSAGES_ENABLED
                std::cerr << "[GL] ERROR In Glgenbuffers: Failed To Generate Index Buffer" << std::endl;
            #endif
            return;
        }
        if(!fill_buffer(indices)){
            #ifdef DEBUG_MESSAGES_ENABLED
                std::cerr << "[GL] ERROR In Glbufferdata: Failed To Fill The Index Buffer" << std::endl;
            #endif
            return;
        }
        #ifdef DEBUG_MESSAGES_ENABLED
            std::cerr << "[GL] Succesfully Created The Index Buffer Object" << std::endl;
        #endif
    }

    ElementBuffer::ElementBuffer(ElementBuffer&& other): ID{other.ID} {
        other.ID = -1;
    }

    ElementBuffer& ElementBuffer::operator=(ElementBuffer&& other){
        this->ID = other.ID;
        other.ID = -1;
        return (*this);
    }

    bool ElementBuffer::bind() const{
        if(this->ID == -1) return false;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ID);
        return true;
    }  

    bool ElementBuffer::unbind() const{
        if(this->ID == -1) return false;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        return true;
    }

    int ElementBuffer::get_ID(){ 
        return this->ID; 
    }

    #ifndef BUFFER_CLEAN_DISABLED
    ElementBuffer::~ElementBuffer() {
        if(this->ID != -1){
            unsigned int buffer = static_cast<unsigned int>(ID);
            glDeleteBuffers(1, &buffer);
        }
    }
    #endif

    bool ElementBuffer::create_ebo(){
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        if(EBO == 0) return false;
        this->ID = static_cast<int>(EBO);
        return true;
    }

    bool ElementBuffer::fill_buffer(const std::vector<unsigned int>& indices){
        if(this->ID == -1) return false;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<unsigned int>(this->ID));
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), indices.data(), GL_STATIC_DRAW);
        return true;
    }
}