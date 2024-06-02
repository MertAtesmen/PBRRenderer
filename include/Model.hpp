#include "Mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <filesystem>
#include <unordered_map>

#include <stb/stb_image.h>

#pragma once

struct TextureHasher
{
    std::size_t operator()(const Texture& texture){
        return std::hash<std::string>{ }(texture.path);
    }
};

class Model
{

public:
    Model(const std::string& path, bool activate_textures = true);
    void draw(const PBR::Shader& shader);
    ~Model();
    
    static unsigned int texture_from_file(const std::string& path);

private:
    std::vector<Mesh> _meshes;
    std::filesystem::path _directory;
    // This is a set for the loaded textures so we dont load the same texture twice
    std::unordered_map<size_t, Texture> _loaded_textures;
    bool activate_textures;

    void load_model(const std::string& path);
    void process_node(const aiScene *scene, aiNode *node);
    void process_mesh(const aiScene *scene, aiMesh *mesh);
    std::vector<Texture> load_material_textures(aiMaterial *mat, aiTextureType type, const std::string& typeName);
};

Model::Model(const std::string& path, bool activate_textures)
    : activate_textures { activate_textures }
{
    load_model(path);

    for(const std::pair<size_t, Texture>& loaded_texture: _loaded_textures){
        std::cout << "Texture type: " << loaded_texture.second.type << ", Texture path: " << loaded_texture.second.path << "\n";
    }
}

inline void Model::draw(const PBR::Shader &shader)
{
    for(Mesh& mesh: _meshes){
        mesh.draw(shader);
    }
}

Model::~Model()
{
}

inline void Model::load_model(const std::string & path)
{
    // Importer takes care of the data structures itself
    // When the Assimp::Importer goes out of scope its destructor will clear all the data it initialized 
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if(scene == nullptr | scene->mRootNode == nullptr | (scene->mFlags && AI_SCENE_FLAGS_INCOMPLETE)){
        std::cerr << "Error loading the Model:\n";
        std::cerr << importer.GetErrorString() << "\n";
        return;
    }

    _directory = path.substr(0, path.find_last_of("/"));

    process_node(scene, scene->mRootNode);
}

inline void Model::process_node(const aiScene *scene, aiNode *node)
{
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        // Node only stores the index to the meshes, scene is the struct that holds the actual meshes 
        process_mesh(scene, scene->mMeshes[node->mMeshes[i]]);
    }
    
    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        process_node(scene, node->mChildren[i]);
    }
    
    
}

inline void Model::process_mesh(const aiScene *scene, aiMesh *mesh)
{   
    // Allocate the vector as mesh->mNumVertices size 
    std::vector<Vertex> vertices{ mesh->mNumVertices };
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        // As the aiVector3D contains only x, y, z values we can  copy the content directly to the Vertex vector
        // This will speed up the copying
        std::memcpy(&(vertices[i].position), &mesh->mVertices[i], sizeof(glm::vec3));
        
        if(mesh->HasNormals())
            std::memcpy(&(vertices[i].normal), &mesh->mNormals[i], sizeof(glm::vec3));

        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            vertices[i].tex_coords.x = mesh->mTextureCoords[0][i].x;
            vertices[i].tex_coords.y = mesh->mTextureCoords[0][i].y;

            std::memcpy(&(vertices[i].tangent), &mesh->mTangents[i], sizeof(glm::vec3));
            std::memcpy(&(vertices[i].bitangent), &mesh->mBitangents[i], sizeof(glm::vec3));

        }

    }

    // Fill the indices buffer
    for(size_t i = 0; i < mesh->mNumFaces; i++){
        indices.push_back(mesh->mFaces[i].mIndices[0]);
        indices.push_back(mesh->mFaces[i].mIndices[1]);
        indices.push_back(mesh->mFaces[i].mIndices[2]);
    }

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // 1. diffuse maps
    std::vector<Texture> diffuse_textures = load_material_textures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.cend(), diffuse_textures.begin(), diffuse_textures.end());
    // 2. specular maps
    std::vector<Texture> specular_textures = load_material_textures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.cend(), specular_textures.begin(), specular_textures.end());
    // 3. normal maps 
    std::vector<Texture> normal_textures = load_material_textures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normal_textures.begin(), normal_textures.end());
    // 4. height maps
    std::vector<Texture> height_textures = load_material_textures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), height_textures.begin(), height_textures.end());

    
    
    // This should use move constructor for the vectors and avoid copying the entire data
    _meshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures), activate_textures);
    
}

inline std::vector<Texture> Model::load_material_textures(aiMaterial *mat, aiTextureType type, const std::string &typeName)
{   
    std::vector<Texture> textures;
    
    for (size_t i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        size_t hash_value = std::hash<const char*>{}(str.C_Str()) ^ std::hash<std::string>{}(typeName);
        if(_loaded_textures.contains(hash_value)){
            textures.push_back(_loaded_textures[hash_value]);
        }
        else{
            Texture texture;
            texture.id = texture_from_file(_directory/str.C_Str());
            texture.type = typeName;
            texture.path = str.C_Str();
            _loaded_textures.insert({hash_value, texture});
            textures.push_back(texture);
        }

        
    }
    
    return textures;
}

inline unsigned int Model::texture_from_file(const std::string &path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, channel_number;
    // stbi_set_flip_vertically_on_load(true);
    void* data = stbi_load(path.c_str(), &width, &height, &channel_number, 0);

    if (data)
    {
        GLenum format;
        if (channel_number == 1)
            format = GL_RED;
        else if (channel_number == 3)
            format = GL_RGB;
        else if (channel_number == 4)
            format = GL_RGBA;            

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
