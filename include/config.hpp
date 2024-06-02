#pragma once

#include <glad/glad.h>
#include <cstddef>

namespace PBR
{
    struct VertexAttribConfig
    {
        unsigned int index;
        int size;
        unsigned int type;
        bool normalized;
        int stride;
        std::ptrdiff_t offset;
    };
    
} // namespace PBR

