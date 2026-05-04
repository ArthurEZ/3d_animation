#include "math.h"
#include "assets.h"
#include "graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <fstream>

// Global variables for input handling
struct InputState {
    bool keys[256] = {};
};

InputState g_input;
AnimatedCharacter g_character;
GridMesh g_grid;
float g_animation_time = 0.0f;
Vec3 g_character_pos = {0.0f, 0.0f, 0.0f};
float g_character_yaw = 0.0f;
int g_idle_animation = -1;
int g_walk_animation = -1;
bool g_character_is_moving = false;

std::string read_text_file(const std::filesystem::path& file_path) {
    std::ifstream file(file_path);
    if (!file) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        g_input.keys[key] = true;
    } else if (action == GLFW_RELEASE) {
        g_input.keys[key] = false;
    }

}

void process_input(float delta_time) {
    constexpr float kMoveSpeed = 5.0f;
    constexpr float kTurnSpeed = 2.0f;

    Vec3 move = {};
    if (g_input.keys[GLFW_KEY_W]) {
        move.z -= 1.0f;
    }
    if (g_input.keys[GLFW_KEY_S]) {
        move.z += 1.0f;
    }
    if (g_input.keys[GLFW_KEY_A]) {
        move.x -= 1.0f;
    }
    if (g_input.keys[GLFW_KEY_D]) {
        move.x += 1.0f;
    }

    g_character_is_moving = length_sq(move) > 1e-5f;
    if (g_character_is_moving) {
        move = normalized(move);
        g_character_yaw = std::atan2(move.x, move.z);
        g_character_pos = g_character_pos + move * (kMoveSpeed * delta_time);
    } else {
        if (g_input.keys[GLFW_KEY_LEFT]) {
            g_character_yaw += kTurnSpeed * delta_time;
        }
        if (g_input.keys[GLFW_KEY_RIGHT]) {
            g_character_yaw -= kTurnSpeed * delta_time;
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Character Animator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load character model
    std::string error;
    auto model_path = find_resource("mixamo/Vanguard By T. Choonyung/Vanguard By T. Choonyung.dae");
    if (model_path.empty()) {
        std::cerr << "Could not find character model" << std::endl;
        glfwTerminate();
        return -1;
    }

    if (!load_animated_character(model_path, g_character, error)) {
        std::cerr << "Failed to load character: " << error << std::endl;
        glfwTerminate();
        return -1;
    }

    auto idle_clip_path = find_resource("mixamo/Great Sword Idle.dae");
    auto walk_clip_path = find_resource("mixamo/Standing Walk Forward.dae");
    if (!idle_clip_path.empty()) {
        AnimatedCharacter::Clip idle_clip;
        if (load_animation_clip(idle_clip_path, idle_clip, error)) {
            g_idle_animation = static_cast<int>(g_character.clips.size());
            g_character.clips.push_back(std::move(idle_clip));
            std::cout << "Loaded idle clip: " << g_character.clips[g_idle_animation].name 
                      << " with " << g_character.clips[g_idle_animation].channels.size() << " channels" << std::endl;
        } else {
            std::cerr << "Failed to load idle clip: " << error << std::endl;
        }
    }
    if (!walk_clip_path.empty()) {
        AnimatedCharacter::Clip walk_clip;
        if (load_animation_clip(walk_clip_path, walk_clip, error)) {
            g_walk_animation = static_cast<int>(g_character.clips.size());
            g_character.clips.push_back(std::move(walk_clip));
            std::cout << "Loaded walk clip: " << g_character.clips[g_walk_animation].name 
                      << " with " << g_character.clips[g_walk_animation].channels.size() << " channels" << std::endl;
        } else {
            std::cerr << "Failed to load walk clip: " << error << std::endl;
        }
    }

    // Setup GPU resources
    setup_character_mesh_gpu(g_character.mesh);
    upload_character_textures(g_character.mesh, error);

    // Create grid floor
    g_grid = create_grid(40.0f, 20);

    auto vertex_shader_path = find_resource("shaders/character.vs");
    auto fragment_shader_path = find_resource("shaders/character.fs");
    if (vertex_shader_path.empty() || fragment_shader_path.empty()) {
        std::cerr << "Could not find shader files" << std::endl;
        glfwTerminate();
        return -1;
    }

    const std::string vertex_shader_source = read_text_file(vertex_shader_path);
    const std::string fragment_shader_source = read_text_file(fragment_shader_path);
    if (vertex_shader_source.empty() || fragment_shader_source.empty()) {
        std::cerr << "Failed to read shader files" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create shader program
    GLuint shader_program = create_shader_program(vertex_shader_source.c_str(), fragment_shader_source.c_str());

    // Main loop
    double last_time = glfwGetTime();
    int frame_count = 0;

    while (!glfwWindowShouldClose(window)) {
        double current_time = glfwGetTime();
        float delta_time = static_cast<float>(current_time - last_time);
        last_time = current_time;

        // Process input
        process_input(delta_time);

        // Accumulate animation time
        g_animation_time += delta_time;

        // Select animation based on movement state
        int selected_animation = g_idle_animation;
        if (g_character_is_moving && g_walk_animation >= 0) {
            selected_animation = g_walk_animation;
        }
        if (selected_animation < 0 && !g_character.clips.empty()) {
            selected_animation = 0;
        }

        // Update animation
        if (selected_animation >= 0 && selected_animation < static_cast<int>(g_character.clips.size())) {
            update_animated_character(g_character, selected_animation, g_animation_time);
        }

        // Clear screen
        clear_screen(0.1f, 0.1f, 0.1f, 1.0f);

        // Setup matrices
        glm::mat4 view = glm::lookAt(
            glm::vec3(g_character_pos.x, g_character_pos.y + 2.0f, g_character_pos.z + 3.0f),
            glm::vec3(g_character_pos.x, g_character_pos.y + 1.0f, g_character_pos.z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);

        // Render grid first
        render_grid(g_grid, view, projection, shader_program);

        // Character world matrix
        glm::mat4 world = glm::identity<glm::mat4>();
        world = glm::translate(world, glm::vec3(g_character_pos.x, g_character_pos.y, g_character_pos.z));
        world = glm::rotate(world, g_character_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        world = glm::scale(world, glm::vec3(0.5f, 0.5f, 0.5f));

        // Render character
        render_character(g_character, world, view, projection, shader_program);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        frame_count++;
        if (frame_count % 60 == 0) {
            const int active_animation = (g_character_is_moving && g_walk_animation >= 0) ? g_walk_animation : g_idle_animation;
            std::string currentName = (active_animation >= 0 && active_animation < static_cast<int>(g_character.clips.size()))
                ? g_character.clips[active_animation].name
                : "N/A";
            std::cout << "FPS: " << (60.0 / delta_time) << " | Animations: " << g_character.clips.size()
                      << " | Current: " << currentName
                      << " | Time: " << g_animation_time
                      << " | Moving: " << (g_character_is_moving ? "yes" : "no") << std::endl;
        }
    }

    // Cleanup
    destroy_grid(g_grid);
    free_character_mesh_gpu(g_character.mesh);
    free_character_textures(g_character.mesh);
    glDeleteProgram(shader_program);

    glfwTerminate();
    return 0;
}
