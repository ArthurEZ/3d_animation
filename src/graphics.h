#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "math.h"
#include "assets.h"
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Grid data structure
struct GridMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint vertex_count = 0;
    GLuint index_count = 0;
};

// Shader program management
GLuint create_shader_program(const char* vertex_src, const char* fragment_src);

// Camera and rendering setup
void setup_camera(const Vec3& camera_pos, const Vec3& target, float aspect_ratio);
void render_character(const AnimatedCharacter& character, const glm::mat4& world_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix, GLuint shader_program);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Grid functions
GridMesh create_grid(float size, int grid_count);
void render_grid(const GridMesh& grid, const glm::mat4& view_matrix, const glm::mat4& projection_matrix, GLuint shader_program);
void destroy_grid(GridMesh& grid);

// Utility functions
void clear_screen(float r, float g, float b, float a);

#endif // GRAPHICS_H
