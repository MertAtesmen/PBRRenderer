#define DEBUG_MESSAGES_ENABLED 
// Enable Debug messages
#define BUFFER_CLEAN_DISABLED
// Disable Cleaning

#include <stb/stb_image_write.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/camera.h>

#include "utilities.hpp"
#include "Texture.hpp"
#include "config.hpp"
#include "Shader.hpp"
#include "VertexArrayObject.hpp"


#include <vector>
#include <iostream>
#include <fstream>