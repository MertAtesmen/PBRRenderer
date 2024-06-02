#pragma once

// #define DEBUG_MESSAGES_ENABLED 
// Enable Debug messages

#include <stb/stb_image.h>
#include <string_view>
#include <filesystem>
#include <string>
#include <iostream>

#include <glad/glad.h>

namespace PBR{
    class Texture2D{
    public:
        using FormatType = GLenum;
    
    // ------ One and only constructor ------
    
        Texture2D(std::string_view path_name){
            if(!texture_load(path_name)){
                // TODO: More comphrensive errors
                #ifdef DEBUG_MESSAGES_ENABLED
                    std::cerr << "ERROR LOADING FILE: " << path_name << std::endl;
                #endif
                return; 
            }
            #ifdef DEBUG_MESSAGES_ENABLED
                std::cout << "SUCCESFULLY LOAD THE TEXTURE: " << path_name << std::endl;
                std::cout << *(this) << std::endl;
            #endif
    
        }
        
    // ------ Deleted constructors ------
    
        Texture2D() = delete;
        Texture2D(Texture2D&) = default;
        Texture2D(const Texture2D&) = default;
        Texture2D(Texture2D&&) = default;
    
    // ------ Member Funcitons ------
    
        bool texture_create(){
        
            // TODO: ADD ERROR CHECKING
    
            unsigned int texture;
            glGenTextures(1, &texture);
            this->ID = texture;
    
            glBindTexture(GL_TEXTURE_2D, texture);
            FormatType internal_format = (channel_number == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width,
                   this->height, 0, internal_format, GL_UNSIGNED_BYTE, this->data);
    
            glGenerateMipmap(GL_TEXTURE_2D);
    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
            glBindTexture(GL_TEXTURE_2D, 0);
    
            // This code should only run if there is no error.  
            stbi_image_free(this->data);
            this->data = nullptr;
    
            return true;
        }

        bool bind(){
            if(this->ID == -1) return false;
            glBindTexture(GL_TEXTURE_2D, this->ID);
            return true;
        }
    
        bool bind_to_location(unsigned int location){
            if(this->ID == -1) return false;
            glActiveTexture(location);
            return bind();
        }
    
        operator bool() { return this->ID != -1;}
    
        ~Texture2D(){
        }
    
        int get_ID(){ return this->ID; }
    
    private: 
    // ------ Member Variables ------
    
        int ID{ -1 };
        int width{ -1 };
        int height{ -1 };
        int channel_number{ -1 };
        void* data{ nullptr };
    
    // ------ Functions ------
    
        bool texture_load(std::string_view path_name){
            stbi_set_flip_vertically_on_load(true);
            this->data = stbi_load(path_name.data(), &width, &height, &channel_number, 0);      
            return this->data != nullptr;
        }

    
    // ------ Utilities ------

        friend std::ostream& operator<<(std::ostream& out, const Texture2D& text){
            out << "Texture{";
            if(text.ID == -1){
                out << "uninitialized";
            }else{
                out << "ID=" << ((text.ID != -1) ? std::to_string(text.ID) : std::string("not bind"))<< ", "
                    << "width=" << text.width << ", "
                    << "height=" << text.height << ", "
                    << "channel_number=" << text.channel_number;
            }

            out << '}';
            return out;
        }
    };

}