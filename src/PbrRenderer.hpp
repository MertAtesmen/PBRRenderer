#include "common.hpp"

#include <map>
#include "Model.hpp"

#include <future>

static glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
static glm::mat4 captureViews[] =
{
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
};

struct Sphere{
    unsigned int VAO;
    unsigned int indexCount;
};

struct DrawSphereContext{
    Sphere sphere_info;
    unsigned int albedo_map;
    unsigned int ao_map;
    unsigned int metallic_map;
    unsigned int normal_map;
    unsigned int roughness_map;
};

struct DrawCubeContext{
    unsigned int cubeVAO;
    unsigned int albedo_map;
    unsigned int ao_map;
    unsigned int metallic_map;
    unsigned int normal_map;
    unsigned int roughness_map;
};
struct DrawModelContext{
    Model& model;
    unsigned int albedo_map;
    unsigned int arm_map;
    unsigned int normal_map;
};

struct EnvironmentContext{
    unsigned int cube_map;
    unsigned int irradiance_map;
    unsigned int prefilter_map;
};

static Sphere createSphere();
static unsigned int createCube();
static unsigned int createSkyBox();
static unsigned int createQuad();

static unsigned int hdr_texture_from_file(std::string_view path);

constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 720;
constexpr const char* WINDOW_NAME = "PBR Renderer";

static void errorCallback(int error, const char* description);
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

class PbrRenderer
{
public:
    Camera camera;
    bool first_mouse{ true };
    bool mouse_captured{ true };
    float lastY{ SCR_WIDTH / 2.0f };
    float lastX{ SCR_HEIGHT / 2.0f };

    /* data */
public:
    PbrRenderer(bool debug_messages = false);
    ~PbrRenderer();    

    void run();

private:
    GLFWwindow* window;
    std::map<std::string, unsigned int> textures;
    std::map<std::string, unsigned int> VAO;
    std::map<std::string, PBR::Shader> shaders;
    std::vector<unsigned int> buffers;


    float deltaTime{ 0.0f };
    float lastFrame{ 0.0f };

    bool DEBUG_MESSAGES;
    
    
private:
    void _init();
    void _init_glfw();
    void _init_IMGUI();
    void _load_GL();
    void _set_callbacks();
    

    void _set_GL_options();
    void _clear_GL_resources();
    void _gen_GL_resourcess();
    void _load_GL_textures();
    void _load_GL_cubemaps();
    
    void _process_input();
    unsigned int _generate_cubemap(std::string_view name, unsigned int texture);
    void _generate_irradiance_map(std::string_view name, unsigned int texture);
    void _generate_prefilter_map(std::string_view name, unsigned int texture);
    void _generate_brdfLUT_texture(std::string_view name);
    unsigned int _load_brdfLUT_texture(std::string_view path);

    
    // This is a one time call function
    void _load_cubemap();
    void _get_face();
    void _print_textures();

    void _draw_sphere(const DrawSphereContext& context, const PBR::Shader& shader, const glm::mat4& model = glm::mat4{ 1.0f } );
    void _draw_cube(const DrawCubeContext& context, const PBR::Shader& shader, const glm::mat4& model = glm::mat4{ 1.0f } );
    void _draw_model(const DrawModelContext& context, const PBR::Shader& shader, const glm::mat4& model = glm::mat4{ 1.0f } );
    void _draw_scene(const std::vector<DrawModelContext>& contexts, const PBR::Shader& shader, const std::vector<glm::mat4> models); 
    void _draw_scene(const std::vector<DrawSphereContext>& contexts, const PBR::Shader& shader, const std::vector<glm::mat4> models); 
    void _draw_spheres(const Sphere& sphere, const PBR::Shader& shader);


    void _set_environment(const EnvironmentContext& context, const PBR::Shader& shader);

};

PbrRenderer::PbrRenderer(bool debug_messages)
    : camera{ glm::vec3{ 0.0f, 0.0f, 3.0f} }, DEBUG_MESSAGES{ debug_messages } 
{
    try
    {
        _init();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error in the constructor of the PbrRenderer\n";
        throw;
    }
    
}

PbrRenderer::~PbrRenderer()
{   
    // Destroy IMGUI Context
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Clear the GL resourcess
    _clear_GL_resources();

    // Clear glfw context and resourcess
    glfwTerminate();
}

inline void PbrRenderer::run()
{
    _print_textures();
    // Reset the framebuffer size and bind the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    glm::vec3 light_dir{ 1.0f, 1.0f, -1.0f };
    glm::vec3 light_dir1{ 1.0f, -1.0f, -1.0f };
    glm::vec3 light_dir2{ -1.0f, 1.0f, -1.0f };
    glm::vec3 light_dir3{ -1.0f, -1.0f, -1.0f };
    glm::vec3 ambient{ 0.1f, 0.1f, 0.1f };
    glm::vec3 diffuse{ 1.0f, 1.0f, 1.0f };
    glm::vec3 specular{ 1.0f, 1.0f, 1.0f };
    glm::vec3 outline_color{ 1.0f, 1.0f, 1.0f };

    glm::vec3 rat_position{ 0.0f, 0.0f, 0.449f};
    glm::vec3 rotation{-90.0f, 0.0f, 0.0f};


    PBR::Shader background_shader = shaders["background_shader"];
    PBR::Shader pbr_model_shader = shaders["pbr_model_shader"];
    PBR::Shader pbr_shader = shaders["pbr_shader"];
    PBR::Shader sphere_shader = shaders["sphere_shader"];

    Sphere sphere = createSphere();
    buffers.emplace_back(sphere.VAO);


    unsigned int gold_albedo_map = textures["gold/albedo_map"];
    unsigned int gold_ao_map = textures["gold/ao_map"];
    unsigned int gold_metallic_map = textures["gold/metallic_map"];
    unsigned int gold_normal_map = textures["gold/normal_map"];
    unsigned int gold_roughness_map = textures["gold/roughness_map"];

    unsigned int rusted_iron_albedo_map = textures["rusted_iron/albedo_map"];
    unsigned int rusted_iron_ao_map = textures["rusted_iron/ao_map"];
    unsigned int rusted_iron_metallic_map = textures["rusted_iron/metallic_map"];
    unsigned int rusted_iron_normal_map = textures["rusted_iron/normal_map"];
    unsigned int rusted_iron_roughness_map = textures["rusted_iron/roughness_map"];


    unsigned int marble_bust_albedo_map = textures["marble_bust/albedo_map"];
    unsigned int marble_bust_arm_map = textures["marble_bust/arm_map"];
    unsigned int marble_bust_normal_map = textures["marble_bust/normal_map"];

    unsigned int rat_albedo_map = textures["rat/albedo_map"];
    unsigned int rat_arm_map = textures["rat/arm_map"];
    unsigned int rat_normal_map = textures["rat/normal_map"];

    unsigned int chair_albedo_map = textures["chair/albedo_map"];
    unsigned int chair_arm_map = textures["chair/arm_map"];
    unsigned int chair_normal_map = textures["chair/normal_map"];

    unsigned int cliff_albedo_map = textures["cliff/albedo_map"];
    unsigned int cliff_arm_map = textures["cliff/arm_map"];
    unsigned int cliff_normal_map = textures["cliff/normal_map"];


    unsigned int hdr_cube_map = textures["hdr_cube_map"];
    unsigned int irradiance_map = textures["irradiance_map"];
    unsigned int prefilter_map = textures["prefilter_map"];
    // unsigned int brdfLUT = textures["brdfLUT"];

    Model rat{ "resources/objects/rat/rat.fbx", false};
    Model chair{ "resources/objects/chair/chair.fbx", false};
    Model bust{ "resources/objects/marble_bust/marble_bust.fbx", false };


    unsigned int cubeVAO = VAO["cubeVAO"];
    unsigned int skyBoxVAO = VAO["skyBoxVAO"];


    _set_environment({hdr_cube_map, irradiance_map, prefilter_map}, pbr_shader);
    _set_environment({hdr_cube_map, irradiance_map, prefilter_map}, pbr_model_shader);
    _set_environment({hdr_cube_map, irradiance_map, prefilter_map}, sphere_shader);

    auto set_lightning = [&](const PBR::Shader& shader, const glm::mat4 view, const glm::mat4 projection){
        shader.use();

        shader.setVec3("light.direction", light_dir);
        shader.setVec3("light.ambient", ambient);
        shader.setVec3("light.diffuse", diffuse);
        shader.setVec3("light.specular", specular);
        shader.setBool("light.isDirLight", true);

        shader.setVec3("lightDirections[0]", light_dir);
        shader.setVec3("lightDirections[1]", light_dir1);
        shader.setVec3("lightDirections[2]", light_dir2);
        shader.setVec3("lightDirections[3]", light_dir3);



        shader.setVec3("viewPos", camera.Position);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
    };



    background_shader.use();
    background_shader.setInt("environmentMap", 5);

    int current_item = 0;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window))
    {

        // Boiler plate code 
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float frame_per_second = 1.0f / deltaTime;
        
        _process_input();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // Create IMGUI new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Actual drawing
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        set_lightning(pbr_model_shader, view, projection);
        set_lightning(pbr_shader, view, projection);
        set_lightning(sphere_shader, view, projection);

        glm::mat4 model{ 1.0f };
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3{ 1.0f, 0.0f, 0.0f });
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3{ 0.0f, 1.0f, 0.0f });
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3{ 1.0f, 0.0f, 1.0f });

        switch (current_item)
        {
        case 0:
            _draw_sphere({sphere, gold_albedo_map, gold_ao_map, gold_metallic_map, gold_normal_map, gold_roughness_map}, pbr_shader, model);
            break;
        case 1:
            _draw_spheres(sphere, sphere_shader);
            break;
        case 2:
            _draw_model({rat,rat_albedo_map,rat_arm_map,rat_normal_map}, pbr_model_shader, model);
            break;
        case 3:
            _draw_scene(
                {
                    {sphere, gold_albedo_map, gold_ao_map, gold_metallic_map, gold_normal_map, gold_roughness_map},
                    {sphere, rusted_iron_albedo_map, rusted_iron_ao_map, rusted_iron_metallic_map, rusted_iron_normal_map, rusted_iron_roughness_map},
                },
                pbr_shader,
                {
                    model,
                    glm::translate(model, glm::vec3{ 3.0f, 0.0f, 0.0f}),
                }
            );
            break;
        case 4:
            _draw_scene(
                {
                    {chair, chair_albedo_map, chair_arm_map, chair_normal_map},
                    {rat, rat_albedo_map, rat_arm_map, rat_normal_map},
                    {bust, marble_bust_albedo_map, marble_bust_arm_map, marble_bust_normal_map}
                },
                pbr_model_shader,
                {
                    glm::scale(model, glm::vec3{ 5.0f, 5.0f, 5.0f }),
                    glm::translate(glm::scale(model, glm::vec3{ 5.0f, 5.0f, 5.0f }), rat_position),
                    glm::translate(glm::scale(model, glm::vec3{ 5.0f, 5.0f, 5.0f }), glm::vec3{ -5.0f, 0.0f, 0.0f })
                }
            );
        case 5:
            
            break;
        default:
            break;
        }

        // Draw the cube map

        view = glm::mat4{ glm::mat3{ camera.GetViewMatrix() } };

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content

        background_shader.use();

        background_shader.setMat4("view", view);
        background_shader.setMat4("projection", projection);

        glBindVertexArray(skyBoxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthFunc(GL_LESS);  // change depth function so depth test passes when values are equal to depth buffer's content




        const char* items[] ={
            "Sphere",
            "Spheres",
            "Model",
            "Textured Spheres",
            "Scene",
            "Nothing",
        };



        // Boiler Plate code 
        ImGui::Begin("Debug Console");
        
        ImGui::Text("FPS: %f", frame_per_second);
        ImGui::DragFloat3("Light Direction", glm::value_ptr(light_dir));
        ImGui::DragFloat3("Light Direction1", glm::value_ptr(light_dir1));
        ImGui::DragFloat3("Light Direction2", glm::value_ptr(light_dir2));
        ImGui::DragFloat3("Light Direction3", glm::value_ptr(light_dir3));
        ImGui::ColorEdit3("Light Ambient", glm::value_ptr(ambient));
        ImGui::ColorEdit3("Light Diffuse", glm::value_ptr(diffuse));
        ImGui::ColorEdit3("Light Specular", glm::value_ptr(specular));
        ImGui::DragFloat3("Object Rotation", glm::value_ptr(rotation));
        ImGui::DragFloat3("Rat Position", glm::value_ptr(rat_position));
        ImGui::ColorEdit3("Outline Color", glm::value_ptr(outline_color));

        ImGui::ListBox("Draw", &current_item, items, 6);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

}

inline void PbrRenderer::_init()
{
    _init_glfw();
    _load_GL();
    _set_callbacks();
    _set_GL_options();
    _gen_GL_resourcess();
    _init_IMGUI();
}

inline void PbrRenderer::_init_glfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    if(window == nullptr){
        std::cerr << "Failed to create GLFW Window\n";
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW Window");
    }

    if(DEBUG_MESSAGES){
        std::cout << "GLFW Init Success\n";
    }

    glfwMakeContextCurrent(window);
}

inline void PbrRenderer::_load_GL()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    if(DEBUG_MESSAGES){
        std::cout << "Glad Loader Success\n";
    }
}

inline void PbrRenderer::_set_callbacks()
{
    // Set the pointer to this class so we can acquire a reference to this class in our callbacks
    glfwSetWindowUserPointer(window, this);

    // Set the callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetErrorCallback(errorCallback);
}

inline void PbrRenderer::_init_IMGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    ImGui_ImplOpenGL3_Init("#version 460 core");

    if(DEBUG_MESSAGES){
        std::cout << "Init IMGUI Success\n";
    }
}

inline void PbrRenderer::_set_GL_options()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(PBR::MessageCallback, nullptr);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glEnable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


}

inline void PbrRenderer::_clear_GL_resources()
{
    // Delete textures
    for (auto &&i : textures)
    {
        glDeleteTextures(1, &i.second);
    }

    // Delete vertex array objects
    for (auto &&i : VAO)
    {
        glDeleteVertexArrays(1, &i.second);
    }

    // Delete shaders
    for (auto &&i : shaders)
    {
        glDeleteProgram(i.second.ID);
    }

    // Delete buffer
    glDeleteBuffers(buffers.size(), buffers.data());
    
}

inline void PbrRenderer::_gen_GL_resourcess()
{
    // ---------- Textures ----------
    _load_GL_textures();

    // ---------- Shaders ----------
    PBR::Shader sphere_shader{ "shaders/sphere_pbr.shader" };
    PBR::Shader e_map_to_cube_map_shader{ "shaders/e_map_to_cube_map.shader" };
    PBR::Shader background_shader{ "shaders/background.shader" };
    PBR::Shader irradiance_shader{ "shaders/irradiance.shader" };
    PBR::Shader pbr_model_shader{ "shaders/pbr_model.shader" };
    PBR::Shader pbr_shader{ "shaders/pbr.shader" };
    PBR::Shader prefilter_shader{ "shaders/prefilter.shader" };
    PBR::Shader brdf_shader{ "shaders/brdf.shader" };

    shaders.insert({"sphere_shader", sphere_shader});
    shaders.insert({"e_map_to_cube_map_shader", e_map_to_cube_map_shader});
    shaders.insert({"background_shader",background_shader});
    shaders.insert({"irradiance_shader", irradiance_shader});
    shaders.insert({"pbr_model_shader", pbr_model_shader});
    shaders.insert({"pbr_shader", pbr_shader});
    shaders.insert({"prefilter_shader", prefilter_shader});
    shaders.insert({"brdf_shader", brdf_shader});

    // ---------- Vertex Attribute Arrays ----------
    unsigned int cubeVAO = createCube();
    unsigned int skyBoxVAO = createSkyBox();
    unsigned int quadVAO = createQuad();

    VAO.insert({"cubeVAO", cubeVAO});
    VAO.insert({"skyBoxVAO", skyBoxVAO});
    VAO.insert({"quadVAO", quadVAO});
    // ---------- Cube Map Textures ----------
    _load_GL_cubemaps();

}

inline void PbrRenderer::_load_GL_textures()
{
    // ---------- Textures ----------
    unsigned int boulder_albedo_map = Model::texture_from_file("resources/objects/boulder/textures/albedo.png");
    glfwPollEvents();
    unsigned int boulder_arm_map = Model::texture_from_file("resources/objects/boulder/textures/arm.png");
    glfwPollEvents();
    unsigned int boulder_normal_map = Model::texture_from_file("resources/objects/boulder/textures/normal.png");
    glfwPollEvents();

    textures.insert({"boulder/albedo_map", boulder_albedo_map});
    textures.insert({"boulder/arm_map", boulder_arm_map});
    textures.insert({"boulder/normal_map", boulder_normal_map});
    
    unsigned int gold_albedo_map = Model::texture_from_file("resources/textures/gold/albedo.png");
    glfwPollEvents();
    unsigned int gold_ao_map = Model::texture_from_file("resources/textures/gold/ao.png");
    glfwPollEvents();
    unsigned int gold_metallic_map = Model::texture_from_file("resources/textures/gold/metallic.png");
    glfwPollEvents();
    unsigned int gold_normal_map = Model::texture_from_file("resources/textures/gold/normal.png");
    glfwPollEvents();
    unsigned int gold_roughness_map = Model::texture_from_file("resources/textures/gold/roughness.png");
    glfwPollEvents();

    textures.insert({"gold/albedo_map", gold_albedo_map});
    textures.insert({"gold/ao_map", gold_ao_map});
    textures.insert({"gold/metallic_map", gold_metallic_map});
    textures.insert({"gold/normal_map", gold_normal_map});
    textures.insert({"gold/roughness_map", gold_roughness_map});

    unsigned int plastic_albedo_map = Model::texture_from_file("resources/textures/plastic/albedo.png");
    glfwPollEvents();
    unsigned int plastic_ao_map = Model::texture_from_file("resources/textures/plastic/ao.png");
    glfwPollEvents();
    unsigned int plastic_metallic_map = Model::texture_from_file("resources/textures/plastic/metallic.png");
    glfwPollEvents();
    unsigned int plastic_normal_map = Model::texture_from_file("resources/textures/plastic/normal.png");
    glfwPollEvents();
    unsigned int plastic_roughness_map = Model::texture_from_file("resources/textures/plastic/roughness.png");
    glfwPollEvents();

    textures.insert({"plastic/albedo_map", plastic_albedo_map});
    textures.insert({"plastic/ao_map", plastic_ao_map});
    textures.insert({"plastic/metallic_map", plastic_metallic_map});
    textures.insert({"plastic/normal_map", plastic_normal_map});
    textures.insert({"plastic/roughness_map", plastic_roughness_map});

    unsigned int cliff_albedo_map = Model::texture_from_file("resources/objects/cliff/textures/albedo.jpg");
    glfwPollEvents();
    unsigned int cliff_arm_map = Model::texture_from_file("resources/objects/cliff/textures/arm.jpg");
    glfwPollEvents();
    unsigned int cliff_normal_map = Model::texture_from_file("resources/objects/cliff/textures/normal.jpg");
    glfwPollEvents();

    textures.insert({"cliff/albedo_map", cliff_albedo_map});
    textures.insert({"cliff/arm_map", cliff_arm_map});
    textures.insert({"cliff/normal_map", cliff_normal_map});

    unsigned int rusted_iron_albedo_map = Model::texture_from_file("resources/textures/rusted_iron/albedo.png");
    glfwPollEvents();
    unsigned int rusted_iron_ao_map = Model::texture_from_file("resources/textures/rusted_iron/ao.png");
    glfwPollEvents();
    unsigned int rusted_iron_metallic_map = Model::texture_from_file("resources/textures/rusted_iron/metallic.png");
    glfwPollEvents();
    unsigned int rusted_iron_normal_map = Model::texture_from_file("resources/textures/rusted_iron/normal.png");
    glfwPollEvents();
    unsigned int rusted_iron_roughness_map = Model::texture_from_file("resources/textures/rusted_iron/roughness.png");
    glfwPollEvents();

    textures.insert({"rusted_iron/albedo_map", rusted_iron_albedo_map});
    textures.insert({"rusted_iron/ao_map", rusted_iron_ao_map});
    textures.insert({"rusted_iron/metallic_map", rusted_iron_metallic_map});
    textures.insert({"rusted_iron/normal_map", rusted_iron_normal_map});
    textures.insert({"rusted_iron/roughness_map", rusted_iron_roughness_map});

    unsigned int marble_bust_arm_map = Model::texture_from_file("resources/objects/marble_bust/textures/arm.jpg");
    glfwPollEvents();
    unsigned int marble_bust_albedo_map = Model::texture_from_file("resources/objects/marble_bust/textures/albedo.jpg");
    glfwPollEvents();
    unsigned int marble_bust_normal_map = Model::texture_from_file("resources/objects/marble_bust/textures/normal.jpg");
    glfwPollEvents();

    textures.insert({"marble_bust/albedo_map", marble_bust_albedo_map});
    textures.insert({"marble_bust/arm_map", marble_bust_arm_map});
    textures.insert({"marble_bust/normal_map", marble_bust_normal_map});

    unsigned int chair_arm_map = Model::texture_from_file("resources/objects/chair/textures/arm.jpg");
    glfwPollEvents();
    unsigned int chair_albedo_map = Model::texture_from_file("resources/objects/chair/textures/albedo.jpg");
    glfwPollEvents();
    unsigned int chair_normal_map = Model::texture_from_file("resources/objects/chair/textures/normal.jpg");
    glfwPollEvents();

    textures.insert({"chair/albedo_map", chair_albedo_map});
    textures.insert({"chair/arm_map", chair_arm_map});
    textures.insert({"chair/normal_map", chair_normal_map});

    unsigned int rat_arm_map = Model::texture_from_file("resources/objects/rat/textures/arm.jpg");
    glfwPollEvents();
    unsigned int rat_albedo_map = Model::texture_from_file("resources/objects/rat/textures/albedo.jpg");
    glfwPollEvents();
    unsigned int rat_normal_map = Model::texture_from_file("resources/objects/rat/textures/normal.jpg");
    glfwPollEvents();

    textures.insert({"rat/albedo_map", rat_albedo_map});
    textures.insert({"rat/arm_map", rat_arm_map});
    textures.insert({"rat/normal_map", rat_normal_map});
    
}

inline void PbrRenderer::_load_GL_cubemaps()
{
    // ---------- Cube Map Textures ----------  
    unsigned int hdr_texture = hdr_texture_from_file("resources/textures/hdr/newport_loft.hdr");
    unsigned int golden_bay_hdr_texture = hdr_texture_from_file("resources/textures/hdr/golden_bay.hdr");
    unsigned int satara_night_hdr_texture = hdr_texture_from_file("resources/textures/hdr/satara_night.hdr");

    textures.insert({"hdr_texture", hdr_texture});
    textures.insert({"golden_bay/hdr_texture", golden_bay_hdr_texture});
    textures.insert({"satara_night/hdr_texture", golden_bay_hdr_texture});

    
    // _generate_brdfLUT_texture("brdfLUT");
    


    _load_cubemap();

    unsigned int hdr_cube_map = _generate_cubemap("hdr_cube_map", hdr_texture);
    unsigned int golden_bay_hdr_cube_map = _generate_cubemap("golden_bay/hdr_cube_map", golden_bay_hdr_texture);
    unsigned int satara_night_hdr_cube_map = _generate_cubemap("satara_night/hdr_cube_map", satara_night_hdr_texture);

    _generate_irradiance_map("golden_bay/irradiance_map", golden_bay_hdr_cube_map);
    _generate_prefilter_map("golden_bay/prefilter_map", golden_bay_hdr_cube_map);
    _generate_irradiance_map("satara_night/irradiance_map", satara_night_hdr_cube_map);
    _generate_prefilter_map("satara_night/prefilter_map", satara_night_hdr_cube_map);
    _generate_irradiance_map("irradiance_map", hdr_cube_map);
    _generate_prefilter_map("prefilter_map", hdr_cube_map);
}

inline void PbrRenderer::_process_input()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS){
        mouse_captured = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if(glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS){
        mouse_captured = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS)
        camera.Position += (glm::vec3(0.0, 1.0, 0.0f) * (2 * deltaTime)); 
    if(glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS)
        camera.Position -= (glm::vec3(0.0, 1.0, 0.0f) * (2 * deltaTime)); 
}

inline unsigned int PbrRenderer::_generate_cubemap(std::string_view name, unsigned int texture)
{

    std::cout << "Generating Cube Map Texture: " << name << std::endl;

    unsigned int captureFBO,captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);



    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    PBR::Shader e_map_to_cube_map_shader = shaders["e_map_to_cube_map_shader"];
    unsigned int skyBoxVAO = VAO["skyBoxVAO"];

    // convert HDR equirectangular environment map to cubemap equivalent
    e_map_to_cube_map_shader.use();
    e_map_to_cube_map_shader.setInt("equirectangularMap", 0);
    e_map_to_cube_map_shader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glViewport(0, 0, 1024, 1024); // don’t forget to configure the viewport
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; ++i)
    {
        e_map_to_cube_map_shader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(skyBoxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    textures.insert({name.data(), envCubemap});

    return envCubemap;
}

inline void PbrRenderer::_generate_irradiance_map(std::string_view name, unsigned int texture)
{

    std::cout << "Generating Irradiance Map Texture: " << name << std::endl;

    unsigned int captureFBO,captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);


    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    PBR::Shader irradiance_shader = shaders["irradiance_shader"];
    unsigned int skyBoxVAO = VAO["skyBoxVAO"];

    irradiance_shader.use();
    irradiance_shader.setInt("environmentMap", 0);
    irradiance_shader.setMat4("projection", captureProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    
    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradiance_shader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(skyBoxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    textures.insert({name.data(), irradianceMap});

}

inline void PbrRenderer::_generate_prefilter_map(std::string_view name, unsigned int texture)
{

    std::cout << "Generating Prefilter Map Texture: " << name << std::endl;

    unsigned int captureFBO,captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);


    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);
    

    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);


    PBR::Shader prefilter_shader = shaders["prefilter_shader"];
    unsigned int skyBoxVAO = VAO["skyBoxVAO"];

    prefilter_shader.use();
    prefilter_shader.setInt("environmentMap", 0);
    prefilter_shader.setMat4("projection", captureProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilter_shader.setFloat("roughness", roughness);
        for (size_t i = 0; i < 6; i++)
        {
            prefilter_shader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(skyBoxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    textures.insert({name.data(), prefilterMap});

}

inline void PbrRenderer::_generate_brdfLUT_texture(std::string_view name)
{

    std::cout << "Generating BRDF LUT Texture: " << name << std::endl;

    unsigned int captureFBO,captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);

    unsigned int brdfLUT_texture;
    glGenTextures(1, &brdfLUT_texture);

    glBindTexture(GL_TEXTURE_2D, brdfLUT_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT_texture, 0);

    PBR::Shader brdf_shader = shaders["brdf_shader"];
    unsigned int quadVAO = VAO["quadVAO"];

    brdf_shader.use();

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    textures.insert({name.data(), brdfLUT_texture});
}

inline unsigned int PbrRenderer::_load_brdfLUT_texture(std::string_view path)
{
    return 0;
}

inline void PbrRenderer::_load_cubemap()
{
    std::cout << "Loading the cube Map Texture: " << "Skybox" << std::endl;
    
    std::vector<std::string> faces{
        "resources/textures/skybox/right.jpg",
        "resources/textures/skybox/left.jpg",
        "resources/textures/skybox/top.jpg",
        "resources/textures/skybox/bottom.jpg",
        "resources/textures/skybox/front.jpg",
        "resources/textures/skybox/back.jpg"
    };

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        stbi_set_flip_vertically_on_load(false);
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    textures.insert({"cube_map", textureID});
}

inline void PbrRenderer::_get_face()
{
    unsigned int textureID = textures["cube_map"];

    std::cout << "Cube_map texture ID: " << textureID << std::endl;

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height;
    glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &width);

    std::cout << "Height: " << height << std::endl;
    std::cout << "Width: " << width << std::endl;

    unsigned char* data = new unsigned char[width * height * 4];

    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    if(stbi_write_png("a.png", width, height, 4, data, width * 4)){
        std::cout << "Successfully wrote " << "a.png" << "\n";
    } else {
        std::cerr << "Failed to write " << "a.png" << "\n";
    }

    delete[] data;

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

inline void PbrRenderer::_print_textures()
{
    for (auto &&i : textures)
    {
        std::cout << "Texture name: " << i.first << ", id: " << i.second << std::endl;
    }
    
}

inline void PbrRenderer::_draw_sphere(const DrawSphereContext &context, const PBR::Shader &shader, const glm::mat4& model)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, context.albedo_map);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, context.ao_map);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, context.metallic_map);

    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, context.normal_map);
    
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, context.roughness_map);

    shader.use();

    shader.setInt("albedo_map", 0);
    shader.setInt("ao_map", 1);
    shader.setInt("metallic_map", 2);
    shader.setInt("normal_map", 3);
    shader.setInt("roughness_map", 4);

    shader.setMat4("model", model);
    
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    shader.setMat3("normalMatrix", normalMatrix);

    glBindVertexArray(context.sphere_info.VAO);
    glDrawElements(GL_TRIANGLE_STRIP, context.sphere_info.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

inline void PbrRenderer::_draw_cube(const DrawCubeContext &context, const PBR::Shader &shader, const glm::mat4 &model)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, context.albedo_map);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, context.ao_map);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, context.metallic_map);

    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, context.normal_map);
    
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, context.roughness_map);

    shader.use();


    shader.setInt("albedo_map", 0);
    shader.setInt("ao_map", 1);
    shader.setInt("metallic_map", 2);
    shader.setInt("normal_map", 3);
    shader.setInt("roughness_map", 4);

    shader.setMat4("model", model);
        
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    shader.setMat3("normalMatrix", normalMatrix);

    glBindVertexArray(context.cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

inline void PbrRenderer::_draw_model(const DrawModelContext &context, const PBR::Shader &shader, const glm::mat4 &model)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, context.albedo_map);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, context.arm_map);

    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, context.normal_map);

    shader.use();

    shader.setInt("albedo_map", 0);
    shader.setInt("arm_map", 1);
    shader.setInt("normal_map", 3);

    shader.setMat4("model", model);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    shader.setMat3("normalMatrix", normalMatrix);

    context.model.draw(shader);
}

inline void PbrRenderer::_draw_scene(const std::vector<DrawModelContext> &contexts, const PBR::Shader &shader, const std::vector<glm::mat4> models)
{
    for (size_t i = 0; i < contexts.size(); i++)
    {
        _draw_model(contexts[i], shader, models[i]);
    }
}

inline void PbrRenderer::_draw_scene(const std::vector<DrawSphereContext>& contexts, const PBR::Shader& shader, const std::vector<glm::mat4> models){
    for (size_t i = 0; i < contexts.size(); i++)
    {
        glm::mat4 model = glm::rotate(models[i], glm::radians(90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });
        _draw_sphere(contexts[i], shader, models[i]);
    }
}

inline void PbrRenderer::_draw_spheres(const Sphere& sphere, const PBR::Shader &shader)
{
    for (size_t i = 0; i < 7; i++)
    {
        for (size_t j = 0; j < 7; j++)
        {
            glm::mat4 model{ 1.0f };
            float roughness = static_cast<float>(j + 0.5f) / 7.0f;
            float metallic = static_cast<float>(6 - i) / 7.0f;
            model = glm::translate(model, glm::vec3{ -6.0f + (3.0f * j), 6.0f -(3.0f * i), 0.0f });

            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

            shader.use();

            shader.setMat4("model", model);
            shader.setMat3("normalMatrix", normalMatrix);

            shader.setVec3("color", 1.0f, 0.0f, 0.0f);
            shader.setFloat("roughness", roughness);
            shader.setFloat("metallic", metallic);


            glBindVertexArray(sphere.VAO);
            glDrawElements(GL_TRIANGLE_STRIP, sphere.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
        
    }
    
}

inline void PbrRenderer::_set_environment(const EnvironmentContext &context, const PBR::Shader &shader)
{
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, context.cube_map);

    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, context.irradiance_map);

    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, context.prefilter_map);

    shader.use();

    shader.setInt("cube_map", 5);
    shader.setInt("irradiance_map", 6);
    shader.setInt("prefilter_map", 7);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{   
    PbrRenderer* renderer = static_cast<PbrRenderer*>(glfwGetWindowUserPointer(window));
    if(!renderer->mouse_captured) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (renderer->first_mouse)
    {
        renderer->lastX = xpos;
        renderer->lastY = ypos;
        renderer->first_mouse = false;
    }

    float xoffset = xpos - renderer->lastX;
    float yoffset = renderer->lastY - ypos; // reversed since y-coordinates go from bottom to top

    renderer->lastX = xpos;
    renderer->lastY = ypos;

    renderer->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    PbrRenderer* renderer = static_cast<PbrRenderer*>(glfwGetWindowUserPointer(window));
    if(!renderer->mouse_captured) return;
    renderer->camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


static Sphere createSphere()
{
    unsigned int sphereVAO;
    glGenVertexArrays(1, &sphereVAO);

    unsigned int vbo, ebo;

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359f;
    for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
    {
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            positions.push_back(glm::vec3(xPos, yPos, zPos));
            uv.push_back(glm::vec2(xSegment, ySegment));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }

    unsigned int indexCount = static_cast<unsigned int>(indices.size());
    std::vector<float> data;
    for (unsigned int i = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);           
        if (normals.size() > 0)
        {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }
        if (uv.size() > 0)
        {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }
    }
    
    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    unsigned int stride = (3 + 2 + 3) * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1);        
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));        

    glBindVertexArray(0);

    return {sphereVAO, indexCount};
}

static unsigned int createCube(){

    unsigned int cubeVAO, cubeVBO;

    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
        // bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
         1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // link vertex attributes
    glBindVertexArray(cubeVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return cubeVAO;
}


static unsigned int createQuad()
{
    unsigned int quadVAO;
    unsigned int quadVBO;
    
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return quadVAO;
}
static unsigned int hdr_texture_from_file(std::string_view path){
    
    stbi_set_flip_vertically_on_load(true);

    int width, height, nrComponents;
    float *data = stbi_loadf(path.data(), &width, &height, &nrComponents, 0);
    unsigned int hdr_texture;
    if (data)
    {
        glGenTextures(1, &hdr_texture);
        glBindTexture(GL_TEXTURE_2D, hdr_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::string error = "Failed to load HDR: ";
        error = error + stbi_failure_reason();
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    return hdr_texture;
}

static unsigned int createSkyBox(){
    unsigned int skyBoxVAO, skyBoxVBO;

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyBoxVAO);
    glGenBuffers(1, &skyBoxVAO);
    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, skyBoxVAO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    // link vertex attributes
    glBindVertexArray(skyBoxVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return skyBoxVAO;
}

static void errorCallback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}