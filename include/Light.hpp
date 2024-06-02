#include <glm/glm.hpp>

#pragma once

namespace PBR{
    struct DirectionalLight
    {
        glm::vec3 direction;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

    struct PointLight
    {
        glm::vec3 position;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        // constant, linear, quadratic
        glm::vec3 attenuationScalars;
    };

    struct SpotLight
    {
        glm::vec3 position;
        glm::vec3 direction;

        float cutoff;
        float outerCutOff;
        
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 attenuationScalars;
    };
    
};