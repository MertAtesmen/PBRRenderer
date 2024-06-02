#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <string>
#include <vector>

#include <glad/glad.h>
#include <Shader.hpp>

#pragma once 

struct Vertex{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct Texture{
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh
{

public:
    Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures, bool activate_textures = true);
    Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, std::vector<Texture>&& textures, bool activate_textures = true);

    Mesh(Mesh&&) = default;
    ~Mesh();
    void draw(const PBR::Shader& shader);

private:
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    std::vector<Texture> _textures;
    unsigned int VAO; // Vertex array object: stores the buffers and vertex format
    unsigned int ABO; // Array buffer object: vertex attributes buffer
    unsigned int EBO; // Element array buffer object: vertex indices buffer
    bool activate_textures;
    

    void _SetupMesh();
};

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures, bool activate_textures)
    :_vertices{ std::move(vertices) }, _indices{ std::move(indices) }, _textures{ std::move(textures) }, activate_textures { activate_textures }
{
    this->_SetupMesh();
}

Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, std::vector<Texture>&& textures, bool activate_textures)
    :_vertices{ std::move(vertices) }, _indices{ std::move(indices) }, _textures{ std::move(textures) }, activate_textures { activate_textures }
{
    this->_SetupMesh();
}


Mesh::~Mesh()
{
}

inline void Mesh::draw(const PBR::Shader &shader)
{
    if(activate_textures){
        unsigned int diffuse_num = 1;
        unsigned int specular_num = 1;
        for (size_t i = 0; i < _textures.size(); i++)
        {
            // Activate the i'th texture
            glActiveTexture(GL_TEXTURE0 + i);

            std::string& texture_name = _textures[i].type;
            std::string number;

            if(texture_name == "texture_diffuse"){
                number = std::to_string(diffuse_num);
                ++diffuse_num;
            }
            else if(texture_name == "texture_specular"){
                number = std::to_string(specular_num);
                ++specular_num;
            }

            // Bind the texture to the i'th slot and change the sampler2D variable in the shader to i
            glBindTexture(GL_TEXTURE_2D, _textures[i].id);
            shader.setInt(texture_name + number, i);
        }
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

}

inline void Mesh::_SetupMesh()
{
    // Create and bind Vertex Array Object
    glCreateVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create and populate the Array Buffer
    glGenBuffers(1, &ABO);
    glBindBuffer(GL_ARRAY_BUFFER, ABO);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(decltype(_vertices)::value_type), _vertices.data(), GL_STATIC_DRAW);

    // Create and populate the Element Array Buffer
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(decltype(_indices)::value_type), _indices.data(), GL_STATIC_DRAW);

    // Change the format of the vertex attributes
    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(decltype(_vertices)::value_type), nullptr); 
    
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(decltype(_vertices)::value_type), reinterpret_cast<void *>(offsetof(Vertex, normal))); 

    // texture cordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(_vertices)::value_type), reinterpret_cast<void *>(offsetof(Vertex, tex_coords))); 

     // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(decltype(_vertices)::value_type), reinterpret_cast<void *>(offsetof(Vertex, tangent)));
    
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(decltype(_vertices)::value_type),  reinterpret_cast<void *>(offsetof(Vertex, bitangent)));

    glBindVertexArray(0);
}


