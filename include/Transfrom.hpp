#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma once

class Transfrom
{
private:
    glm::mat4 _transform;
public:
    explicit Transfrom(glm::vec3 position = glm::vec3{ 0.0f }, glm::vec3 scale = glm::vec3{ 1.0f });
    
    glm::mat4& translate(glm::vec3 translation);
    glm::mat4& scale(glm::vec3 scale);
    glm::mat4& rotate(float radians, glm::vec3 axis);
    glm::mat4& rotateX(float radians);
    glm::mat4& rotateY(float radians);
    glm::mat4& rotateZ(float radians);
    glm::mat4& get();

    ~Transfrom
    ();
};

Transfrom::Transfrom(glm::vec3 position, glm::vec3 scale)  
    :_transform{1.0f}
{
    
    _transform = glm::translate(_transform, position);
    _transform = glm::scale(_transform, scale);
}

glm::mat4 & Transfrom::translate(glm::vec3 translation)
{
    _transform = glm::translate(_transform, translation);
    return _transform;
}

glm::mat4 &Transfrom::scale(glm::vec3 scale)
{
    _transform = glm::translate(_transform, scale);
    return _transform;
}

inline glm::mat4 &Transfrom::rotate(float radians, glm::vec3 axis)
{
    _transform = glm::rotate(_transform, radians, axis);
    return _transform;
}

inline glm::mat4 &Transfrom::rotateX(float radians)
{
    return this->rotate(radians, glm::vec3{1.0f, 0.0f, 0.0f});
}

inline glm::mat4 &Transfrom::rotateY(float radians)
{
    return this->rotate(radians, glm::vec3{0.0f, 1.0f, 0.0f});
}

inline glm::mat4 &Transfrom::rotateZ(float radians)
{
    return this->rotate(radians, glm::vec3{0.0f, 0.0f, 1.0f});
}

inline glm::mat4 &Transfrom::get()
{
    return _transform;
}

Transfrom::~Transfrom()
{
}
